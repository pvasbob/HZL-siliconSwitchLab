#pragma once

#include "silicon_switch/asic/bounded_queue.hpp"
#include "silicon_switch/asic/switch_asic.hpp"
#include "silicon_switch/routing/arp_cache.hpp"
#include "silicon_switch/routing/ipv4_route_table.hpp"
#include "silicon_switch/switching/l2_l3_pipeline.hpp"
#include "silicon_switch/switching/mac_table.hpp"
#include "silicon_switch/switching/virtual_port.hpp"

#include <map>
#include <memory>
#include <set>
#include <vector>

namespace silicon_switch::asic {

class SoftwareAsic final : public SwitchAsic {
public:
    explicit SoftwareAsic(std::size_t mac_table_capacity = 4'096U);

    AsicStatus create_port(switching::VirtualPort port,std::size_t queue_capacity) override;
    AsicStatus remove_port(routing::PortId port) override;
    AsicStatus set_port_state(routing::PortId port,bool admin_enabled,bool link_up) override;
    AsicStatus create_vlan(network::VlanId vlan) override;
    AsicStatus remove_vlan(network::VlanId vlan) override;
    AsicStatus add_vlan_member(network::VlanId vlan,routing::PortId port) override;
    AsicStatus set_router_interface(network::VlanId vlan,network::MacAddress mac) override;
    AsicStatus add_or_replace_route(routing::RouteEntry route) override;
    AsicStatus remove_route(network::Ipv4Prefix prefix) override;
    AsicStatus add_or_replace_neighbor(network::Ipv4Address address,network::MacAddress mac,
                                       switching::MacTable::TimePoint learned_at) override;
    AsicStatus remove_neighbor(network::Ipv4Address address) override;
    AsicStatus configure_faults(FaultInjectionConfig config) override;
    bool has_port(routing::PortId port) const override;
    bool has_vlan(network::VlanId vlan) const override;
    std::optional<routing::RouteEntry> find_route(network::Ipv4Prefix prefix) const override;
    std::optional<network::MacAddress> find_neighbor(
        network::Ipv4Address address,
        switching::MacTable::TimePoint now) const override;
    PacketProcessingResult process_packet(routing::PortId ingress_port,
                                           std::vector<std::uint8_t> frame,
                                           switching::MacTable::TimePoint now) override;
    std::optional<std::vector<std::uint8_t>> dequeue_packet(routing::PortId port) override;
    TrafficStatisticsSnapshot counters() const noexcept override;

private:
    using PacketQueue = BoundedQueue<std::vector<std::uint8_t>>;

    PacketProcessingResult enqueue_outputs(
        PacketDisposition disposition,
        const std::vector<routing::PortId>& ports,
        const std::vector<std::uint8_t>& frame,
        std::chrono::nanoseconds ingress_latency);
    void count_pipeline_drop(switching::PipelineDropReason reason) noexcept;

    std::map<routing::PortId,switching::VirtualPort> ports_;
    std::map<routing::PortId,std::unique_ptr<PacketQueue>> queues_;
    std::set<network::VlanId> vlans_;
    std::map<network::VlanId,std::vector<routing::PortId>> vlan_members_;
    switching::RouterInterfaces router_interfaces_;
    switching::MacTable mac_table_;
    routing::Ipv4RouteTable route_table_;
    routing::ArpCache arp_cache_;
    FaultInjector fault_injector_;
    TrafficStatistics statistics_;
};

}  // namespace silicon_switch::asic
