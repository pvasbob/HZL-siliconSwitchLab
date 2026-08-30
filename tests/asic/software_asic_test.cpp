#include "asic/software_asic_test.hpp"
#include "silicon_switch/asic/programmable_switch.hpp"
#include "silicon_switch/asic/software_asic.hpp"
#include "silicon_switch/asic/traffic_statistics.hpp"
#include "silicon_switch/network/ether_type.hpp"
#include "silicon_switch/network/ethernet_frame.hpp"
#include "silicon_switch/network/ip_protocol.hpp"
#include "silicon_switch/network/ipv4_address.hpp"
#include "silicon_switch/network/ipv4_packet.hpp"
#include "silicon_switch/network/ipv4_prefix.hpp"
#include "silicon_switch/network/mac_address.hpp"
#include "silicon_switch/network/vlan_id.hpp"
#include "silicon_switch/routing/port_id.hpp"
#include "silicon_switch/routing/route_entry.hpp"
#include "silicon_switch/switching/mac_table.hpp"
#include "silicon_switch/switching/virtual_port.hpp"
#include "silicon_switch/switching/vlan_port_config.hpp"
#include "test_support.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace silicon_switch::test { namespace {
network::VlanId vlan(std::uint16_t v){return *network::VlanId::create(v);}
routing::PortId port(std::uint16_t v){return *routing::PortId::create(v);}
network::MacAddress mac(network::MacAddress::Bytes b){return network::MacAddress{b};}
network::Ipv4Address ip(std::uint32_t v){return network::Ipv4Address{v};}
switching::VirtualPort make_port(std::uint16_t id,network::MacAddress address){
    return *switching::VirtualPort::create(port(id),address,switching::PortSpeed::gbps_10,
        1500U,switching::VlanPortConfig::access(vlan(10U)));
}
network::EthernetFrame frame(network::MacAddress source,network::MacAddress destination,
                             network::EtherType type=network::EtherType::ipv4,
                             std::vector<std::uint8_t> payload={1U}){
    return *network::EthernetFrame::create(destination,source,type,std::move(payload));
}
} void run_software_asic_tests(TestSuite& suite) {
    const auto host_a=mac({0x02U,0U,0U,0U,0U,0x0AU});
    const auto host_b=mac({0x02U,0U,0U,0U,0U,0x0BU});
    const auto router=mac({0x02U,0U,0U,0U,0U,0xFEU});
    asic::ProgrammableSwitch api{std::make_unique<asic::SoftwareAsic>(32U)};
    suite.expect_equal(api.create_vlan(vlan(10U)),asic::AsicStatus::success,
                       "software ASIC creates VLAN");
    suite.expect_equal(api.create_vlan(vlan(10U)),asic::AsicStatus::already_exists,
                       "software ASIC rejects duplicate VLAN");
    suite.expect_equal(api.create_port(make_port(1U,router),4U),asic::AsicStatus::success,
                       "software ASIC creates first port");
    suite.expect_equal(api.create_port(make_port(2U,router),4U),asic::AsicStatus::success,
                       "software ASIC creates second port");
    suite.expect_equal(api.create_port(make_port(3U,router),4U),asic::AsicStatus::success,
                       "software ASIC creates third port");
    for(std::uint16_t id=1U;id<=3U;++id){
        suite.expect_equal(api.set_port_state(port(id),true,true),asic::AsicStatus::success,
                           "software ASIC enables port");
        suite.expect_equal(api.add_vlan_member(vlan(10U),port(id)),asic::AsicStatus::success,
                           "software ASIC adds VLAN member");
    }
    suite.expect_equal(api.set_router_interface(vlan(10U),router),asic::AsicStatus::success,
                       "software ASIC configures router interface");
    suite.expect_true(api.has_vlan(vlan(10U)),"query configured VLAN");
    suite.expect_true(api.has_port(port(1U)),"query configured port");
    const switching::MacTable::TimePoint now{};

    const auto learn_frame=frame(host_b,mac({0xFFU,0xFFU,0xFFU,0xFFU,0xFFU,0xFFU}));
    const auto flooded=api.process_packet(port(2U),learn_frame.serialize(),now);
    suite.expect_equal(flooded.disposition,asic::PacketDisposition::flooded,
                       "software pipeline floods broadcast");
    suite.expect_equal(flooded.output_ports,std::vector<routing::PortId>{port(1U),port(3U)},
                       "software pipeline floods to VLAN peers");
    suite.expect_true(api.dequeue_packet(port(1U)).has_value(),"dequeue first flooded frame");
    suite.expect_true(api.dequeue_packet(port(3U)).has_value(),"dequeue second flooded frame");

    const auto switched=api.process_packet(port(1U),frame(host_a,host_b).serialize(),now);
    suite.expect_equal(switched.disposition,asic::PacketDisposition::switched,
                       "software pipeline switches learned unicast");
    suite.expect_equal(switched.output_ports,std::vector<routing::PortId>{port(2U)},
                       "software pipeline selects learned egress");
    const auto switched_bytes=api.dequeue_packet(port(2U));
    suite.expect_true(switched_bytes.has_value(),"dequeue switched frame");

    const auto route_prefix=*network::Ipv4Prefix::create(ip(0xCB007100U),24U);
    const auto route=*routing::RouteEntry::create(route_prefix,ip(0xC0A80102U),port(2U));
    suite.expect_equal(api.add_or_replace_route(route),asic::AsicStatus::success,
                       "software ASIC programs IPv4 route");
    suite.expect_equal(api.add_or_replace_neighbor(ip(0xC0A80102U),host_b,now),
                       asic::AsicStatus::success,"software ASIC programs neighbor");
    suite.expect_equal(api.find_route(route_prefix).value(),route,"query programmed route");
    suite.expect_equal(api.find_neighbor(ip(0xC0A80102U),now).value(),host_b,
                       "query programmed neighbor");
    const auto ipv4=*network::Ipv4Packet::create(ip(0xC0A8010AU),ip(0xCB007109U),
        network::IpProtocol::udp,{0x12U,0x34U},64U);
    const auto routed=api.process_packet(port(1U),
        frame(host_a,router,network::EtherType::ipv4,ipv4.serialize()).serialize(),now);
    suite.expect_equal(routed.disposition,asic::PacketDisposition::routed,
                       "software pipeline routes IPv4 packet");
    const auto routed_bytes=api.dequeue_packet(port(2U));
    suite.expect_true(routed_bytes.has_value(),"dequeue routed Ethernet frame");
    if(routed_bytes){
        const auto routed_frame=network::EthernetFrame::parse(*routed_bytes);
        suite.expect_true(routed_frame.has_value(),"parse routed Ethernet frame");
        if(routed_frame){
            suite.expect_equal(routed_frame->source(),router,"rewrite routed source MAC");
            suite.expect_equal(routed_frame->destination(),host_b,"rewrite routed destination MAC");
            const auto routed_packet=network::Ipv4Packet::parse(routed_frame->payload());
            suite.expect_true(routed_packet.has_value(),"parse routed IPv4 payload");
            if(routed_packet) suite.expect_equal(routed_packet->time_to_live(),std::uint8_t{63U},
                                                 "decrement TTL in complete pipeline");
        }
    }
    const auto counters=api.counters();
    suite.expect_equal(counters.get(asic::TrafficCounter::ingress_packets),std::uint64_t{3U},
                       "complete pipeline counts ingress packets");
    suite.expect_equal(counters.get(asic::TrafficCounter::egress_packets),std::uint64_t{4U},
                       "complete pipeline counts egress packets");

    suite.expect_equal(api.remove_route(route_prefix),asic::AsicStatus::success,
                       "software ASIC removes route");
    suite.expect_equal(api.remove_neighbor(ip(0xC0A80102U)),asic::AsicStatus::success,
                       "software ASIC removes neighbor");
    suite.expect_equal(api.remove_vlan(vlan(10U)),asic::AsicStatus::dependency_missing,
                       "software ASIC protects VLAN dependencies");
} }
