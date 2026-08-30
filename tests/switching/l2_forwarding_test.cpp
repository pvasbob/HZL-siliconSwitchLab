#include "switching/l2_forwarding_test.hpp"
#include "silicon_switch/network/ethernet_frame.hpp"
#include "silicon_switch/network/mac_address.hpp"
#include "silicon_switch/network/vlan_id.hpp"
#include "silicon_switch/routing/port_id.hpp"
#include "silicon_switch/switching/l2_forwarding.hpp"
#include "silicon_switch/switching/mac_table.hpp"
#include "test_support.hpp"
#include <cstdint>
#include <vector>

namespace silicon_switch::test { namespace {
network::MacAddress mac(network::MacAddress::Bytes b) { return network::MacAddress{b}; }
network::VlanId vlan(std::uint16_t v) { return *network::VlanId::create(v); }
routing::PortId port(std::uint16_t v) { return *routing::PortId::create(v); }
network::EthernetFrame frame_to(network::MacAddress destination) {
    return *network::EthernetFrame::create(destination, mac({0x02U,0U,0U,0U,0U,9U}),
        network::EtherType::ipv4, {1U});
}
} void run_l2_forwarding_tests(TestSuite& suite) {
    switching::MacTable table{8U};
    const auto known = mac({0x02U,0U,0U,0U,0U,2U});
    suite.expect_equal(table.learn(vlan(10U), known, port(2U)), switching::MacTableUpdate::inserted,
                       "learn known L2 destination");
    const std::vector<routing::PortId> members{port(1U),port(2U),port(3U),port(3U)};
    const auto unicast = switching::decide_l2_forwarding(frame_to(known),vlan(10U),port(1U),members,table);
    suite.expect_equal(unicast.action(), switching::L2ForwardingAction::known_unicast,
                       "forward known unicast");
    suite.expect_equal(unicast.output_ports(), std::vector<routing::PortId>{port(2U)},
                       "select learned unicast port");
    const auto unknown = switching::decide_l2_forwarding(
        frame_to(mac({0x02U,0U,0U,0U,0U,8U})),vlan(10U),port(1U),members,table);
    suite.expect_equal(unknown.action(), switching::L2ForwardingAction::unknown_unicast_flood,
                       "flood unknown unicast");
    suite.expect_equal(unknown.output_ports(), std::vector<routing::PortId>{port(2U),port(3U)},
                       "flood once excluding ingress port");
    const auto broadcast = switching::decide_l2_forwarding(
        frame_to(mac({0xFFU,0xFFU,0xFFU,0xFFU,0xFFU,0xFFU})),vlan(10U),port(1U),members,table);
    suite.expect_equal(broadcast.action(), switching::L2ForwardingAction::broadcast_flood,
                       "flood broadcast");
    const auto multicast = switching::decide_l2_forwarding(
        frame_to(mac({0x01U,0U,0x5EU,0U,0U,1U})),vlan(10U),port(1U),members,table);
    suite.expect_equal(multicast.action(), switching::L2ForwardingAction::multicast_flood,
                       "flood multicast");
    suite.expect_equal(table.learn(vlan(10U), mac({0x02U,0U,0U,0U,0U,4U}), port(1U)),
                       switching::MacTableUpdate::inserted, "learn destination on ingress port");
    const auto filtered = switching::decide_l2_forwarding(
        frame_to(mac({0x02U,0U,0U,0U,0U,4U})),vlan(10U),port(1U),members,table);
    suite.expect_equal(filtered.drop_reason().value(), switching::L2ForwardingDropReason::same_ingress_port,
                       "filter destination on ingress port");
    switching::MacTable outside{2U};
    suite.expect_equal(outside.learn(vlan(10U),known,port(9U)),switching::MacTableUpdate::inserted,
                       "learn destination outside VLAN members");
    suite.expect_equal(switching::decide_l2_forwarding(frame_to(known),vlan(10U),port(1U),members,outside)
                           .drop_reason().value(),
                       switching::L2ForwardingDropReason::destination_not_in_vlan,
                       "drop learned destination outside VLAN");
    suite.expect_equal(switching::decide_l2_forwarding(frame_to(mac({0x02U,0U,0U,0U,0U,8U})),
                           vlan(10U),port(1U),{port(1U)},table).drop_reason().value(),
                       switching::L2ForwardingDropReason::no_eligible_egress_port,
                       "drop flood without eligible egress");
} }
