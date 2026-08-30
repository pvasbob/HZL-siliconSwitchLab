#include "switching/virtual_port_test.hpp"
#include "silicon_switch/network/mac_address.hpp"
#include "silicon_switch/network/vlan_id.hpp"
#include "silicon_switch/routing/port_id.hpp"
#include "silicon_switch/switching/virtual_port.hpp"
#include "silicon_switch/switching/vlan_port_config.hpp"
#include "test_support.hpp"
#include <cstddef>
#include <cstdint>
#include <utility>

namespace silicon_switch::test { namespace {
network::VlanId vlan(std::uint16_t v) { return *network::VlanId::create(v); }
routing::PortId port(std::uint16_t v) { return *routing::PortId::create(v); }
network::MacAddress mac(network::MacAddress::Bytes b) { return network::MacAddress{b}; }
} void run_virtual_port_tests(TestSuite& suite) {
    const auto address=mac({0x02U,0U,0U,0U,0U,1U});
    auto created=switching::VirtualPort::create(port(1U),address,switching::PortSpeed::gbps_10,
                                                1500U,switching::VlanPortConfig::access(vlan(10U)));
    suite.expect_true(created.has_value(),"create virtual port");
    if (!created) { return; }
    auto virtual_port=std::move(*created);
    suite.expect_equal(virtual_port.id(),port(1U),"store virtual port identifier");
    suite.expect_equal(virtual_port.mac_address(),address,"store virtual port MAC");
    suite.expect_equal(virtual_port.speed(),switching::PortSpeed::gbps_10,"store port speed");
    suite.expect_equal(virtual_port.mtu(),std::size_t{1500U},"store port MTU");
    suite.expect_false(virtual_port.operational(),"new virtual port starts down");
    suite.expect_false(virtual_port.receive(100U),"drop receive while port down");
    suite.expect_false(virtual_port.transmit(100U),"drop transmit while port down");
    virtual_port.set_admin_enabled(true);
    suite.expect_false(virtual_port.operational(),"admin up requires link up");
    virtual_port.set_link_up(true);
    suite.expect_true(virtual_port.operational(),"port operates when admin and link are up");
    suite.expect_true(virtual_port.receive(100U),"count successful receive");
    suite.expect_true(virtual_port.transmit(200U),"count successful transmit");
    suite.expect_false(virtual_port.receive(1501U),"drop oversized receive");
    suite.expect_false(virtual_port.transmit(1501U),"drop oversized transmit");
    virtual_port.record_receive_error(); virtual_port.record_transmit_error();
    const auto counters=virtual_port.counters();
    suite.expect_equal(counters.received_packets,std::uint64_t{1U},"count received packets");
    suite.expect_equal(counters.received_bytes,std::uint64_t{100U},"count received bytes");
    suite.expect_equal(counters.transmitted_packets,std::uint64_t{1U},"count transmitted packets");
    suite.expect_equal(counters.transmitted_bytes,std::uint64_t{200U},"count transmitted bytes");
    suite.expect_equal(counters.receive_drops,std::uint64_t{2U},"count receive drops");
    suite.expect_equal(counters.transmit_drops,std::uint64_t{2U},"count transmit drops");
    suite.expect_equal(counters.receive_errors,std::uint64_t{2U},"count receive errors");
    suite.expect_equal(counters.transmit_errors,std::uint64_t{2U},"count transmit errors");
    suite.expect_false(virtual_port.set_mtu(63U),"reject undersized MTU");
    suite.expect_true(virtual_port.set_mtu(9000U),"update valid MTU");
    virtual_port.set_speed(switching::PortSpeed::gbps_100);
    suite.expect_equal(virtual_port.speed(),switching::PortSpeed::gbps_100,"update port speed");
    virtual_port.set_vlan_config(switching::VlanPortConfig::access(vlan(20U)));
    suite.expect_equal(virtual_port.vlan_config().access_vlan().value(),vlan(20U),
                       "update virtual port VLAN configuration");
    virtual_port.reset_counters();
    suite.expect_equal(virtual_port.counters(),switching::PortCounters{},"reset port counters");
    suite.expect_false(switching::VirtualPort::create(port(1U),mac({0U,0U,0U,0U,0U,0U}),
        switching::PortSpeed::gbps_1,1500U,switching::VlanPortConfig::access(vlan(10U))).has_value(),
        "reject invalid virtual port MAC");
    suite.expect_false(switching::VirtualPort::create(port(1U),address,switching::PortSpeed::gbps_1,
        10'000U,switching::VlanPortConfig::access(vlan(10U))).has_value(),
        "reject invalid virtual port MTU");
} }
