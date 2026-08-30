#include "silicon_switch/asic/programmable_switch.hpp"

#include <stdexcept>
#include <utility>

namespace silicon_switch::asic {

ProgrammableSwitch::ProgrammableSwitch(std::unique_ptr<SwitchAsic> asic)
    : asic_{std::move(asic)} {
    if (!asic_) { throw std::invalid_argument{"switch ASIC cannot be null"}; }
}

AsicStatus ProgrammableSwitch::create_port(switching::VirtualPort p,std::size_t q){return asic_->create_port(std::move(p),q);}
AsicStatus ProgrammableSwitch::remove_port(routing::PortId p){return asic_->remove_port(p);}
AsicStatus ProgrammableSwitch::set_port_state(routing::PortId p,bool a,bool l){return asic_->set_port_state(p,a,l);}
AsicStatus ProgrammableSwitch::create_vlan(network::VlanId v){return asic_->create_vlan(v);}
AsicStatus ProgrammableSwitch::remove_vlan(network::VlanId v){return asic_->remove_vlan(v);}
AsicStatus ProgrammableSwitch::add_vlan_member(network::VlanId v,routing::PortId p){return asic_->add_vlan_member(v,p);}
AsicStatus ProgrammableSwitch::set_router_interface(network::VlanId v,network::MacAddress m){return asic_->set_router_interface(v,m);}
AsicStatus ProgrammableSwitch::add_or_replace_route(routing::RouteEntry r){return asic_->add_or_replace_route(std::move(r));}
AsicStatus ProgrammableSwitch::remove_route(network::Ipv4Prefix p){return asic_->remove_route(p);}
AsicStatus ProgrammableSwitch::add_or_replace_neighbor(network::Ipv4Address a,network::MacAddress m,switching::MacTable::TimePoint t){return asic_->add_or_replace_neighbor(a,m,t);}
AsicStatus ProgrammableSwitch::remove_neighbor(network::Ipv4Address a){return asic_->remove_neighbor(a);}
AsicStatus ProgrammableSwitch::configure_faults(FaultInjectionConfig c){return asic_->configure_faults(std::move(c));}
bool ProgrammableSwitch::has_port(routing::PortId p) const{return asic_->has_port(p);}
bool ProgrammableSwitch::has_vlan(network::VlanId v) const{return asic_->has_vlan(v);}
std::optional<routing::RouteEntry> ProgrammableSwitch::find_route(network::Ipv4Prefix p) const{return asic_->find_route(p);}
std::optional<network::MacAddress> ProgrammableSwitch::find_neighbor(network::Ipv4Address a,switching::MacTable::TimePoint t) const{return asic_->find_neighbor(a,t);}
PacketProcessingResult ProgrammableSwitch::process_packet(routing::PortId p,std::vector<std::uint8_t> f,switching::MacTable::TimePoint t){return asic_->process_packet(p,std::move(f),t);}
std::optional<std::vector<std::uint8_t>> ProgrammableSwitch::dequeue_packet(routing::PortId p){return asic_->dequeue_packet(p);}
TrafficStatisticsSnapshot ProgrammableSwitch::counters() const noexcept{return asic_->counters();}

}  // namespace silicon_switch::asic
