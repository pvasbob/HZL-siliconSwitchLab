#include "silicon_switch/asic/software_asic.hpp"

#include "silicon_switch/network/arp_packet.hpp"
#include "silicon_switch/network/ethernet_frame.hpp"
#include "silicon_switch/network/ipv4_packet.hpp"
#include "silicon_switch/routing/ipv4_forwarding_engine.hpp"
#include "silicon_switch/routing/ipv4_forwarding_result.hpp"
#include "silicon_switch/routing/l3_ethernet_encapsulation.hpp"

#include <algorithm>
#include <chrono>
#include <optional>
#include <stdexcept>
#include <utility>
#include <variant>

namespace silicon_switch::asic {
namespace {

PacketProcessingResult dropped(const PacketDropReason reason,
                               const std::chrono::nanoseconds latency = {}) {
    return {PacketDisposition::dropped,reason,{},latency};
}

bool valid_router_mac(const network::MacAddress& mac) {
    constexpr network::MacAddress::Bytes zero{};
    return mac != network::MacAddress{zero} && mac.is_unicast();
}

}  // namespace

SoftwareAsic::SoftwareAsic(const std::size_t capacity) : mac_table_{capacity} {}

AsicStatus SoftwareAsic::create_port(switching::VirtualPort port,const std::size_t capacity) {
    if (capacity == 0U) return AsicStatus::invalid_argument;
    if (ports_.find(port.id()) != ports_.end()) return AsicStatus::already_exists;
    const auto id=port.id();
    queues_.emplace(id,std::make_unique<PacketQueue>(capacity));
    ports_.emplace(id,std::move(port));
    return AsicStatus::success;
}

AsicStatus SoftwareAsic::remove_port(const routing::PortId port) {
    const auto existing=ports_.find(port);
    if (existing==ports_.end()) return AsicStatus::not_found;
    for (auto& members:vlan_members_) {
        auto& list=members.second;
        list.erase(std::remove(list.begin(),list.end(),port),list.end());
    }
    static_cast<void>(queues_.at(port)->close());
    queues_.erase(port); ports_.erase(existing);
    return AsicStatus::success;
}

AsicStatus SoftwareAsic::set_port_state(const routing::PortId port,const bool admin,const bool link) {
    const auto existing=ports_.find(port);
    if(existing==ports_.end()) return AsicStatus::not_found;
    existing->second.set_admin_enabled(admin); existing->second.set_link_up(link);
    return AsicStatus::success;
}

AsicStatus SoftwareAsic::create_vlan(const network::VlanId vlan) {
    return vlans_.insert(vlan).second?AsicStatus::success:AsicStatus::already_exists;
}

AsicStatus SoftwareAsic::remove_vlan(const network::VlanId vlan) {
    if(vlans_.find(vlan)==vlans_.end()) return AsicStatus::not_found;
    if(!vlan_members_[vlan].empty() || router_interfaces_.find(vlan)!=router_interfaces_.end())
        return AsicStatus::dependency_missing;
    vlan_members_.erase(vlan); vlans_.erase(vlan); return AsicStatus::success;
}

AsicStatus SoftwareAsic::add_vlan_member(const network::VlanId vlan,const routing::PortId port) {
    if(vlans_.find(vlan)==vlans_.end() || ports_.find(port)==ports_.end())
        return AsicStatus::dependency_missing;
    auto& members=vlan_members_[vlan];
    if(std::find(members.begin(),members.end(),port)!=members.end()) return AsicStatus::already_exists;
    if(!ports_.at(port).vlan_config().allows(vlan)) return AsicStatus::invalid_argument;
    members.push_back(port); return AsicStatus::success;
}

AsicStatus SoftwareAsic::set_router_interface(const network::VlanId vlan,network::MacAddress mac) {
    if(vlans_.find(vlan)==vlans_.end()) return AsicStatus::dependency_missing;
    if(!valid_router_mac(mac)) return AsicStatus::invalid_argument;
    router_interfaces_.insert_or_assign(vlan,std::move(mac)); return AsicStatus::success;
}

AsicStatus SoftwareAsic::add_or_replace_route(routing::RouteEntry route) {
    if(ports_.find(route.output_port())==ports_.end()) return AsicStatus::dependency_missing;
    static_cast<void>(route_table_.add_or_replace(std::move(route))); return AsicStatus::success;
}
AsicStatus SoftwareAsic::remove_route(const network::Ipv4Prefix prefix) {
    return route_table_.remove(prefix)?AsicStatus::success:AsicStatus::not_found;
}
AsicStatus SoftwareAsic::add_or_replace_neighbor(const network::Ipv4Address address,
    network::MacAddress mac,const switching::MacTable::TimePoint learned_at) {
    return arp_cache_.add_or_replace(address,std::move(mac),learned_at)==routing::ArpCacheUpdate::rejected
        ?AsicStatus::invalid_argument:AsicStatus::success;
}
AsicStatus SoftwareAsic::remove_neighbor(const network::Ipv4Address address) {
    return arp_cache_.remove(address)?AsicStatus::success:AsicStatus::not_found;
}
AsicStatus SoftwareAsic::configure_faults(FaultInjectionConfig config) {
    try { fault_injector_.set_config(std::move(config)); }
    catch(const std::invalid_argument&) { return AsicStatus::invalid_argument; }
    return AsicStatus::success;
}
bool SoftwareAsic::has_port(const routing::PortId port) const{return ports_.find(port)!=ports_.end();}
bool SoftwareAsic::has_vlan(const network::VlanId vlan) const{return vlans_.find(vlan)!=vlans_.end();}
std::optional<routing::RouteEntry> SoftwareAsic::find_route(const network::Ipv4Prefix prefix) const{return route_table_.find_exact(prefix);}
std::optional<network::MacAddress> SoftwareAsic::find_neighbor(const network::Ipv4Address address,const switching::MacTable::TimePoint now) const{return arp_cache_.lookup(address,now);}

PacketProcessingResult SoftwareAsic::process_packet(const routing::PortId ingress_port,
    std::vector<std::uint8_t> bytes,const switching::MacTable::TimePoint now) {
    const auto port=ports_.find(ingress_port);
    if(port==ports_.end() || !port->second.operational())
        return dropped(PacketDropReason::ingress_port_unavailable);
    statistics_.increment(TrafficCounter::ingress_packets);
    statistics_.increment(TrafficCounter::ingress_bytes,bytes.size());
    auto fault=fault_injector_.process(ingress_port,std::move(bytes));
    if(const auto* reason=std::get_if<FaultDropReason>(&fault)) {
        statistics_.increment(*reason==FaultDropReason::resource_exhaustion
            ?TrafficCounter::resource_exhaustion:TrafficCounter::fault_drops);
        return dropped(*reason==FaultDropReason::resource_exhaustion
            ?PacketDropReason::resource_exhaustion:PacketDropReason::injected_fault);
    }
    auto injected=std::get<FaultInjectedPacket>(std::move(fault));
    if(injected.corrupted()) statistics_.increment(TrafficCounter::corrupted_packets);
    const auto frame=network::EthernetFrame::parse(injected.bytes());
    if(!frame) { statistics_.increment(TrafficCounter::parse_errors);
        return dropped(PacketDropReason::malformed_frame,injected.latency()); }
    if(!port->second.receive(frame->payload().size()))
        return dropped(PacketDropReason::ingress_port_unavailable,injected.latency());
    const auto members=vlan_members_.find(port->second.vlan_config().is_access()
        ?*port->second.vlan_config().access_vlan()
        :frame->vlan_tag().has_value()?frame->vlan_tag()->vlan_id()
        :port->second.vlan_config().native_vlan().value_or(
            *port->second.vlan_config().allowed_vlans().begin()));
    const std::vector<routing::PortId> empty_members;
    const auto& vlan_ports=members==vlan_members_.end()?empty_members:members->second;
    const auto pipeline=switching::process_l2_l3_ingress(*frame,ingress_port,
        port->second.vlan_config(),vlan_ports,mac_table_,router_interfaces_,now);
    if(pipeline.action()==switching::PipelineAction::drop) {
        count_pipeline_drop(*pipeline.drop_reason());
        return dropped(PacketDropReason::vlan_rejected,injected.latency());
    }
    if(pipeline.action()==switching::PipelineAction::deliver_to_control_plane) {
        const auto arp=network::ArpPacket::parse(frame->payload());
        if(!arp) { statistics_.increment(TrafficCounter::parse_errors);
            return dropped(PacketDropReason::malformed_frame,injected.latency()); }
        static_cast<void>(arp_cache_.add_or_replace(arp->sender_ip(),arp->sender_mac(),now));
        return {PacketDisposition::delivered_to_control_plane,PacketDropReason::none,{},injected.latency()};
    }
    if(pipeline.action()==switching::PipelineAction::route_ipv4) {
        const auto packet=network::Ipv4Packet::parse(frame->payload());
        if(!packet) { statistics_.increment(TrafficCounter::parse_errors);
            return dropped(PacketDropReason::malformed_ipv4,injected.latency()); }
        routing::Ipv4ForwardingEngine engine{route_table_};
        const auto forwarding=engine.forward(*packet);
        if(const auto* drop=std::get_if<routing::DroppedIpv4Packet>(&forwarding)) {
            if(drop->reason()==routing::Ipv4DropReason::route_not_found) {
                statistics_.increment(TrafficCounter::route_misses);
                return dropped(PacketDropReason::route_miss,injected.latency());
            }
            statistics_.increment(TrafficCounter::ttl_expirations);
            return dropped(PacketDropReason::ttl_expired,injected.latency());
        }
        const auto& forwarded=std::get<routing::ForwardedIpv4Packet>(forwarding);
        const auto egress=ports_.find(forwarded.output_port());
        if(egress==ports_.end() || !egress->second.operational())
            return dropped(PacketDropReason::egress_port_unavailable,injected.latency());
        const auto encapsulated=routing::encapsulate_ipv4_in_ethernet(
            forwarded,egress->second.mac_address(),arp_cache_,now);
        if(const auto* failure=std::get_if<routing::L3EncapsulationFailure>(&encapsulated)) {
            if(*failure==routing::L3EncapsulationFailure::neighbor_not_found)
                statistics_.increment(TrafficCounter::neighbor_misses);
            return dropped(*failure==routing::L3EncapsulationFailure::neighbor_not_found
                ?PacketDropReason::neighbor_miss:PacketDropReason::egress_port_unavailable,
                injected.latency());
        }
        return enqueue_outputs(PacketDisposition::routed,{forwarded.output_port()},
            std::get<network::EthernetFrame>(encapsulated).serialize(),injected.latency());
    }
    return enqueue_outputs(
        pipeline.action()==switching::PipelineAction::flood
            ?PacketDisposition::flooded:PacketDisposition::switched,
        pipeline.output_ports(),frame->serialize(),injected.latency());
}

PacketProcessingResult SoftwareAsic::enqueue_outputs(PacketDisposition disposition,
    const std::vector<routing::PortId>& requested,const std::vector<std::uint8_t>& frame,
    std::chrono::nanoseconds latency) {
    std::vector<routing::PortId> outputs;
    for(const auto id:requested) {
        auto port=ports_.find(id); auto queue=queues_.find(id);
        if(port==ports_.end() || queue==queues_.end() || !port->second.operational()) continue;
        auto fault=fault_injector_.process(id,frame);
        if(const auto* reason=std::get_if<FaultDropReason>(&fault)) {
            statistics_.increment(*reason==FaultDropReason::resource_exhaustion
                ?TrafficCounter::resource_exhaustion:TrafficCounter::fault_drops); continue;
        }
        auto packet=std::get<FaultInjectedPacket>(std::move(fault));
        latency=std::max(latency,packet.latency());
        if(packet.corrupted()) statistics_.increment(TrafficCounter::corrupted_packets);
        const auto size=packet.bytes().size();
        if(size>port->second.mtu()) {
            static_cast<void>(port->second.transmit(size));
            continue;
        }
        if(queue->second->try_enqueue(packet.bytes())!=QueueEnqueueResult::enqueued) {
            statistics_.increment(TrafficCounter::queue_drops); continue;
        }
        static_cast<void>(port->second.transmit(size));
        statistics_.increment(TrafficCounter::egress_packets);
        statistics_.increment(TrafficCounter::egress_bytes,size);
        outputs.push_back(id);
    }
    if(outputs.empty()) return dropped(PacketDropReason::queue_congestion,latency);
    return {disposition,PacketDropReason::none,std::move(outputs),latency};
}

std::optional<std::vector<std::uint8_t>> SoftwareAsic::dequeue_packet(const routing::PortId port) {
    const auto queue=queues_.find(port); return queue==queues_.end()?std::nullopt:queue->second->try_dequeue();
}
TrafficStatisticsSnapshot SoftwareAsic::counters() const noexcept{return statistics_.snapshot();}

void SoftwareAsic::count_pipeline_drop(const switching::PipelineDropReason reason) noexcept {
    switch(reason) {
        case switching::PipelineDropReason::tagged_frame_on_access_port:
        case switching::PipelineDropReason::untagged_frame_without_native_vlan:
        case switching::PipelineDropReason::vlan_not_allowed:
            statistics_.increment(TrafficCounter::vlan_drops); break;
        default: statistics_.increment(TrafficCounter::filtered_frames); break;
    }
}

}  // namespace silicon_switch::asic
