#include "routing/ipv4_ttl_test.hpp"

#include "silicon_switch/network/ip_protocol.hpp"
#include "silicon_switch/network/ipv4_address.hpp"
#include "silicon_switch/network/ipv4_packet.hpp"
#include "silicon_switch/routing/ipv4_forwarding_result.hpp"
#include "silicon_switch/routing/ipv4_ttl.hpp"
#include "test_support.hpp"

#include <cstdint>
#include <variant>
#include <vector>

namespace silicon_switch::test {
namespace {

network::Ipv4Packet packet_with_ttl(const std::uint8_t time_to_live) {
    return *network::Ipv4Packet::create(
        network::Ipv4Address{0xC0A8010AU},
        network::Ipv4Address{0xCB007109U},
        network::IpProtocol::udp,
        std::vector<std::uint8_t>{0x12U, 0x34U, 0x56U},
        time_to_live,
        0xA1B2U,
        false);
}

}  // namespace

void run_ipv4_ttl_tests(TestSuite& suite) {
    const auto original = packet_with_ttl(64U);
    const auto original_bytes = original.serialize();
    const auto result = routing::decrement_ipv4_ttl(original);

    suite.expect_true(
        std::holds_alternative<network::Ipv4Packet>(result),
        "decrement forwardable IPv4 TTL");
    suite.expect_equal(original.time_to_live(), std::uint8_t{64U},
                       "TTL update leaves input packet unchanged");
    suite.expect_equal(original.serialize(), original_bytes,
                       "TTL update preserves input serialization");

    if (std::holds_alternative<network::Ipv4Packet>(result)) {
        const auto& updated = std::get<network::Ipv4Packet>(result);
        suite.expect_equal(updated.time_to_live(), std::uint8_t{63U},
                           "reduce IPv4 TTL by exactly one");
        suite.expect_equal(updated.source(), original.source(),
                           "TTL update preserves source address");
        suite.expect_equal(updated.destination(), original.destination(),
                           "TTL update preserves destination address");
        suite.expect_equal(updated.protocol(), original.protocol(),
                           "TTL update preserves protocol");
        suite.expect_equal(updated.payload(), original.payload(),
                           "TTL update preserves payload");
        suite.expect_equal(updated.identification(), original.identification(),
                           "TTL update preserves identification");
        suite.expect_equal(updated.dont_fragment(), original.dont_fragment(),
                           "TTL update preserves fragmentation policy");

        const auto reparsed = network::Ipv4Packet::parse(updated.serialize());
        suite.expect_true(reparsed.has_value(),
                          "serialize TTL update with valid checksum");
        if (reparsed.has_value()) {
            suite.expect_equal(*reparsed, updated,
                               "round trip TTL-updated IPv4 packet");
        }
    }

    const auto boundary_result = routing::decrement_ipv4_ttl(packet_with_ttl(2U));
    suite.expect_true(
        std::holds_alternative<network::Ipv4Packet>(boundary_result),
        "forward IPv4 packet when TTL is two");
    if (std::holds_alternative<network::Ipv4Packet>(boundary_result)) {
        suite.expect_equal(
            std::get<network::Ipv4Packet>(boundary_result).time_to_live(),
            std::uint8_t{1U},
            "decrement IPv4 TTL from two to one");
    }

    const auto expired = packet_with_ttl(1U);
    const auto expired_bytes = expired.serialize();
    const auto expired_result = routing::decrement_ipv4_ttl(expired);
    suite.expect_true(
        std::holds_alternative<routing::DroppedIpv4Packet>(expired_result),
        "drop IPv4 packet when TTL is one");
    if (std::holds_alternative<routing::DroppedIpv4Packet>(expired_result)) {
        suite.expect_equal(
            std::get<routing::DroppedIpv4Packet>(expired_result).reason(),
            routing::Ipv4DropReason::time_to_live_expired,
            "report TTL-expired drop reason");
    }
    suite.expect_equal(expired.serialize(), expired_bytes,
                       "TTL drop leaves input packet unchanged");
}

}  // namespace silicon_switch::test
