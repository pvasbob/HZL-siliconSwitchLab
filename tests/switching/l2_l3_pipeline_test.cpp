#include "switching/l2_l3_pipeline_test.hpp"
#include "silicon_switch/network/ether_type.hpp"
#include "silicon_switch/network/ethernet_frame.hpp"
#include "silicon_switch/network/mac_address.hpp"
#include "silicon_switch/network/vlan_id.hpp"
#include "silicon_switch/network/vlan_tag.hpp"
#include "silicon_switch/routing/port_id.hpp"
#include "silicon_switch/switching/l2_l3_pipeline.hpp"
#include "silicon_switch/switching/mac_table.hpp"
#include "silicon_switch/switching/vlan_port_config.hpp"
#include "test_support.hpp"
#include <cstdint>
#include <optional>
#include <vector>

namespace silicon_switch::test { namespace {
network::MacAddress mac(network::MacAddress::Bytes b) { return network::MacAddress{b}; }
network::VlanId vlan(std::uint16_t v) { return *network::VlanId::create(v); }
routing::PortId port(std::uint16_t v) { return *routing::PortId::create(v); }
network::EthernetFrame make_frame(network::MacAddress source, network::MacAddress destination,
                                  network::EtherType type = network::EtherType::ipv4,
                                  std::optional<network::VlanId> tagged_vlan = std::nullopt) {
    std::optional<network::VlanTag> tag;
    if (tagged_vlan) { tag = network::VlanTag::create(*tagged_vlan); }
    return *network::EthernetFrame::create(destination,source,type,{1U},tag);
}
} void run_l2_l3_pipeline_tests(TestSuite& suite) {
    const auto host_a=mac({0x02U,0U,0U,0U,0U,1U});
    const auto host_b=mac({0x02U,0U,0U,0U,0U,2U});
    const auto router=mac({0x02U,0U,0U,0U,0U,0xFEU});
    const auto config=switching::VlanPortConfig::access(vlan(10U));
    const std::vector<routing::PortId> members{port(1U),port(2U),port(3U)};
    switching::RouterInterfaces interfaces{{vlan(10U),router}};
    switching::MacTable table{8U};
    const switching::MacTable::TimePoint now{};
    suite.expect_equal(table.learn(vlan(10U),host_b,port(2U),now),switching::MacTableUpdate::inserted,
                       "prelearn pipeline destination");
    const auto switched=switching::process_l2_l3_ingress(
        make_frame(host_a,host_b),port(1U),config,members,table,interfaces,now);
    suite.expect_equal(switched.action(),switching::PipelineAction::switch_known_unicast,
                       "pipeline selects Layer 2 switching");
    suite.expect_equal(switched.output_ports(),std::vector<routing::PortId>{port(2U)},
                       "pipeline returns switched egress port");
    suite.expect_true(table.lookup(vlan(10U),host_a).has_value(),
                      "pipeline learns accepted source MAC");
    const auto flooded=switching::process_l2_l3_ingress(
        make_frame(host_a,mac({0x02U,0U,0U,0U,0U,9U})),port(1U),config,members,table,interfaces,now);
    suite.expect_equal(flooded.action(),switching::PipelineAction::flood,
                       "pipeline selects unknown-unicast flooding");
    const auto routed=switching::process_l2_l3_ingress(
        make_frame(host_a,router),port(1U),config,members,table,interfaces,now);
    suite.expect_equal(routed.action(),switching::PipelineAction::route_ipv4,
                       "pipeline selects IPv4 routing for router MAC");
    suite.expect_equal(routed.vlan().value(),vlan(10U),"pipeline preserves effective VLAN");
    const auto arp=switching::process_l2_l3_ingress(
        make_frame(host_a,router,network::EtherType::arp),port(1U),config,members,table,interfaces,now);
    suite.expect_equal(arp.action(),switching::PipelineAction::deliver_to_control_plane,
                       "pipeline delivers router ARP locally");
    const auto unsupported=switching::process_l2_l3_ingress(
        make_frame(host_a,router,network::EtherType::ipv6),port(1U),config,members,table,interfaces,now);
    suite.expect_equal(unsupported.drop_reason().value(),
                       switching::PipelineDropReason::unsupported_router_protocol,
                       "pipeline drops unsupported router protocol");
    const auto vlan_drop=switching::process_l2_l3_ingress(
        make_frame(host_a,host_b,network::EtherType::ipv4,vlan(10U)),port(1U),config,members,
        table,interfaces,now);
    suite.expect_equal(vlan_drop.drop_reason().value(),
                       switching::PipelineDropReason::tagged_frame_on_access_port,
                       "pipeline propagates VLAN ingress drop");
    suite.expect_false(vlan_drop.vlan().has_value(),"VLAN failure has no effective VLAN");
    const auto invalid_source=switching::process_l2_l3_ingress(
        make_frame(mac({0x01U,0U,0U,0U,0U,1U}),host_b),port(1U),config,members,
        table,interfaces,now);
    suite.expect_equal(invalid_source.drop_reason().value(),
                       switching::PipelineDropReason::invalid_source_mac,
                       "pipeline rejects multicast source MAC");
    switching::MacTable static_table{2U};
    suite.expect_equal(static_table.add_static(vlan(10U),host_a,port(2U)),
                       switching::MacTableUpdate::inserted,"configure pipeline static source");
    const auto conflict=switching::process_l2_l3_ingress(
        make_frame(host_a,host_b),port(1U),config,members,static_table,interfaces,now);
    suite.expect_equal(conflict.drop_reason().value(),switching::PipelineDropReason::static_mac_conflict,
                       "pipeline rejects static MAC movement");
} }
