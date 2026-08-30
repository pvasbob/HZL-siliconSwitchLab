#include "asic/fault_injector_test.hpp"
#include "silicon_switch/asic/fault_injector.hpp"
#include "silicon_switch/routing/port_id.hpp"
#include "test_support.hpp"
#include <chrono>
#include <cstdint>
#include <stdexcept>
#include <variant>
#include <vector>

namespace silicon_switch::test { namespace {
routing::PortId port(std::uint16_t v) { return *routing::PortId::create(v); }
} void run_fault_injector_tests(TestSuite& suite) {
    using namespace std::chrono_literals;
    asic::FaultInjectionConfig latency_config;
    latency_config.latency=5ms;
    asic::FaultInjector latency{latency_config};
    const auto ready=latency.process(port(1U),{0x10U,0x20U});
    const auto* packet=std::get_if<asic::FaultInjectedPacket>(&ready);
    suite.expect_true(packet!=nullptr,"pass packet through fault injector");
    if (packet) {
        suite.expect_equal(packet->bytes(),std::vector<std::uint8_t>{0x10U,0x20U},
                           "preserve packet without corruption fault");
        suite.expect_equal(packet->latency(),std::chrono::nanoseconds{5ms},
                           "report injected latency without sleeping");
        suite.expect_false(packet->corrupted(),"report uncorrupted packet");
    }

    asic::FaultInjectionConfig failed_config;
    failed_config.failed_ports.insert(port(2U));
    asic::FaultInjector failed{failed_config};
    suite.expect_equal(std::get<asic::FaultDropReason>(failed.process(port(2U),{1U})),
                       asic::FaultDropReason::failed_port,"drop packet on failed port");
    suite.expect_true(std::holds_alternative<asic::FaultInjectedPacket>(failed.process(port(1U),{1U})),
                      "preserve healthy port during port failure");

    asic::FaultInjectionConfig loss_config;
    loss_config.drop_every_nth_packet=2U;
    asic::FaultInjector loss{loss_config};
    suite.expect_true(std::holds_alternative<asic::FaultInjectedPacket>(loss.process(port(1U),{1U})),
                      "deterministic loss passes first packet");
    suite.expect_equal(std::get<asic::FaultDropReason>(loss.process(port(1U),{2U})),
                       asic::FaultDropReason::deterministic_packet_loss,
                       "deterministic loss drops configured packet");
    suite.expect_true(std::holds_alternative<asic::FaultInjectedPacket>(loss.process(port(1U),{3U})),
                      "deterministic loss passes following packet");
    loss.reset_sequence();
    suite.expect_equal(loss.processed_packets(),std::uint64_t{0U},"reset fault sequence");

    asic::FaultInjectionConfig corruption_config;
    corruption_config.corrupt_every_nth_packet=2U;
    asic::FaultInjector corruption{corruption_config};
    static_cast<void>(corruption.process(port(1U),{0x10U}));
    const auto corrupted=std::get<asic::FaultInjectedPacket>(corruption.process(port(1U),{0x10U,0x20U}));
    suite.expect_true(corrupted.corrupted(),"report deterministic packet corruption");
    suite.expect_equal(corrupted.bytes(),std::vector<std::uint8_t>{0x11U,0x20U},
                       "corrupt packet deterministically");

    asic::FaultInjectionConfig exhausted_config;
    exhausted_config.resource_exhausted=true;
    asic::FaultInjector exhausted{exhausted_config};
    suite.expect_equal(std::get<asic::FaultDropReason>(exhausted.process(port(1U),{1U})),
                       asic::FaultDropReason::resource_exhaustion,
                       "drop packet during resource exhaustion");

    asic::FaultInjectionConfig replacement;
    replacement.drop_every_nth_packet=1U;
    latency.set_config(replacement);
    suite.expect_equal(std::get<asic::FaultDropReason>(latency.process(port(1U),{1U})),
                       asic::FaultDropReason::deterministic_packet_loss,
                       "replace fault injection configuration");

    bool rejected=false;
    try { asic::FaultInjectionConfig invalid; invalid.latency=-1ns; const asic::FaultInjector bad{invalid};
          static_cast<void>(bad); }
    catch (const std::invalid_argument&) { rejected=true; }
    suite.expect_true(rejected,"reject negative injected latency");
} }
