#include "asic/programmable_switch_test.hpp"
#include "silicon_switch/asic/mock_switch_asic.hpp"
#include "silicon_switch/asic/programmable_switch.hpp"
#include "silicon_switch/network/mac_address.hpp"
#include "silicon_switch/network/vlan_id.hpp"
#include "silicon_switch/routing/port_id.hpp"
#include "silicon_switch/switching/virtual_port.hpp"
#include "silicon_switch/switching/vlan_port_config.hpp"
#include "test_support.hpp"
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <utility>

namespace silicon_switch::test { namespace {
network::VlanId vlan(std::uint16_t v){return *network::VlanId::create(v);}
routing::PortId port(std::uint16_t v){return *routing::PortId::create(v);}
network::MacAddress mac(network::MacAddress::Bytes b){return network::MacAddress{b};}
switching::VirtualPort virtual_port(){return *switching::VirtualPort::create(port(1U),
    mac({0x02U,0U,0U,0U,0U,1U}),switching::PortSpeed::gbps_1,1500U,
    switching::VlanPortConfig::access(vlan(10U)));}
} void run_programmable_switch_tests(TestSuite& suite) {
    auto mock=std::make_unique<asic::MockSwitchAsic>();
    auto* observed=mock.get();
    asic::ProgrammableSwitch api{std::move(mock)};
    suite.expect_equal(api.create_vlan(vlan(10U)),asic::AsicStatus::success,
                       "programmable API delegates VLAN creation");
    suite.expect_equal(api.create_port(virtual_port(),4U),asic::AsicStatus::success,
                       "programmable API delegates port creation");
    suite.expect_equal(api.add_vlan_member(vlan(10U),port(1U)),asic::AsicStatus::success,
                       "programmable API delegates VLAN membership");
    suite.expect_equal(observed->operation_count(),std::size_t{3U},
                       "mock ASIC records delegated operations");
    observed->set_status(asic::AsicStatus::resource_exhausted);
    suite.expect_equal(api.remove_port(port(1U)),asic::AsicStatus::resource_exhausted,
                       "programmable API preserves hardware error");
    observed->set_packet_result({asic::PacketDisposition::routed,asic::PacketDropReason::none,
                                 {port(1U)},std::chrono::nanoseconds{7}});
    const auto result=api.process_packet(port(1U),{1U},switching::MacTable::TimePoint{});
    suite.expect_equal(result.disposition,asic::PacketDisposition::routed,
                       "programmable API returns mock packet result");
    suite.expect_equal(result.injected_latency,std::chrono::nanoseconds{7},
                       "runtime-polymorphic packet result preserves latency");
    bool rejected=false;
    try { const asic::ProgrammableSwitch invalid{std::unique_ptr<asic::SwitchAsic>{}};
          static_cast<void>(invalid); }
    catch(const std::invalid_argument&) { rejected=true; }
    suite.expect_true(rejected,"reject null ASIC ownership");
} }
