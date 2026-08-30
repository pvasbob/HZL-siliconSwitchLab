#include "silicon_switch/asic/mock_switch_asic.hpp"

namespace silicon_switch::asic {

AsicStatus MockSwitchAsic::create_port(switching::VirtualPort,std::size_t){return record();}
AsicStatus MockSwitchAsic::remove_port(routing::PortId){return record();}
AsicStatus MockSwitchAsic::set_port_state(routing::PortId,bool,bool){return record();}
AsicStatus MockSwitchAsic::create_vlan(network::VlanId){return record();}
AsicStatus MockSwitchAsic::remove_vlan(network::VlanId){return record();}
AsicStatus MockSwitchAsic::add_vlan_member(network::VlanId,routing::PortId){return record();}
AsicStatus MockSwitchAsic::set_router_interface(network::VlanId,network::MacAddress){return record();}
AsicStatus MockSwitchAsic::add_or_replace_route(routing::RouteEntry){return record();}
AsicStatus MockSwitchAsic::remove_route(network::Ipv4Prefix){return record();}
AsicStatus MockSwitchAsic::add_or_replace_neighbor(network::Ipv4Address,network::MacAddress,switching::MacTable::TimePoint){return record();}
AsicStatus MockSwitchAsic::remove_neighbor(network::Ipv4Address){return record();}
AsicStatus MockSwitchAsic::configure_faults(FaultInjectionConfig){return record();}
PacketProcessingResult MockSwitchAsic::process_packet(routing::PortId,std::vector<std::uint8_t>,switching::MacTable::TimePoint){++operation_count_;return packet_result_;}
std::optional<std::vector<std::uint8_t>> MockSwitchAsic::dequeue_packet(routing::PortId){++operation_count_;return std::nullopt;}
TrafficStatisticsSnapshot MockSwitchAsic::counters() const noexcept{return statistics_.snapshot();}

}  // namespace silicon_switch::asic
