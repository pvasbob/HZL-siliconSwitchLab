#include "network/ipv4_packet_test.hpp"

#include "silicon_switch/network/internet_checksum.hpp"
#include "silicon_switch/network/ip_protocol.hpp"
#include "silicon_switch/network/ipv4_address.hpp"
#include "silicon_switch/network/ipv4_packet.hpp"
#include "test_support.hpp"

#include <cstdint>
#include <vector>

namespace silicon_switch::test {
namespace {

using network::IpProtocol;
using network::Ipv4Address;
using network::Ipv4Packet;

Ipv4Address address(const std::uint32_t value) {
    return Ipv4Address{value};
}

}  // namespace

void run_ipv4_packet_tests(TestSuite& suite) {
    const Ipv4Packet::Payload payload{0xDEU, 0xADU, 0xBEU, 0xEFU};
    const auto packet = Ipv4Packet::create(
        address(0xC0A80101U),
        address(0xC0A80102U),
        IpProtocol::udp,
        payload,
        32U,
        0x1234U,
        true);
    suite.expect_true(packet.has_value(), "create IPv4 packet");
    if (!packet.has_value()) {
        return;
    }

    suite.expect_equal(packet->source(), address(0xC0A80101U),
                       "preserve IPv4 source address");
    suite.expect_equal(packet->destination(), address(0xC0A80102U),
                       "preserve IPv4 destination address");
    suite.expect_equal(packet->protocol(), IpProtocol::udp,
                       "preserve IPv4 protocol");
    suite.expect_equal(packet->payload(), payload, "preserve IPv4 payload");
    suite.expect_equal(packet->time_to_live(), std::uint8_t{32U},
                       "preserve IPv4 TTL");
    suite.expect_equal(packet->identification(), std::uint16_t{0x1234U},
                       "preserve IPv4 identification");
    suite.expect_true(packet->dont_fragment(),
                      "preserve IPv4 don't-fragment flag");

    const auto bytes = packet->serialize();
    suite.expect_equal(bytes.size(), std::size_t{24U},
                       "serialize IPv4 total length");
    suite.expect_equal(bytes[0], std::uint8_t{0x45U},
                       "serialize IPv4 version and header length");
    suite.expect_equal(bytes[2], std::uint8_t{0x00U},
                       "serialize IPv4 total-length high byte");
    suite.expect_equal(bytes[3], std::uint8_t{0x18U},
                       "serialize IPv4 total-length low byte");
    suite.expect_true(
        network::has_valid_internet_checksum(
            std::span<const std::uint8_t>{bytes}.first(Ipv4Packet::header_size)),
        "serialize valid IPv4 header checksum");

    const auto parsed = Ipv4Packet::parse(bytes);
    suite.expect_true(parsed.has_value(), "parse serialized IPv4 packet");
    if (parsed.has_value()) {
        suite.expect_equal(*parsed, *packet, "round-trip IPv4 packet");
    }

    const auto default_packet = Ipv4Packet::create(
        address(0x0A000001U),
        address(0x0A000002U),
        IpProtocol::icmp,
        {});
    suite.expect_true(default_packet.has_value(),
                      "create empty-payload IPv4 packet");
    if (default_packet.has_value()) {
        suite.expect_equal(
            default_packet->time_to_live(),
            Ipv4Packet::default_time_to_live,
            "use default IPv4 TTL");
    }

    suite.expect_false(
        Ipv4Packet::create(
            address(1U), address(2U), IpProtocol::tcp, {}, 0U)
            .has_value(),
        "reject zero IPv4 TTL during creation");

    auto malformed = bytes;
    malformed[0] = 0x65U;
    suite.expect_false(Ipv4Packet::parse(malformed).has_value(),
                       "reject non-IPv4 version");

    malformed = bytes;
    malformed[0] = 0x46U;
    suite.expect_false(Ipv4Packet::parse(malformed).has_value(),
                       "reject unsupported IPv4 options");

    malformed = bytes;
    malformed[3] = 0x17U;
    suite.expect_false(Ipv4Packet::parse(malformed).has_value(),
                       "reject mismatched IPv4 total length");

    malformed = bytes;
    malformed[8] ^= 0x01U;
    suite.expect_false(Ipv4Packet::parse(malformed).has_value(),
                       "reject invalid IPv4 header checksum");

    malformed = bytes;
    malformed[6] = 0x20U;
    malformed[10] = 0U;
    malformed[11] = 0U;
    const auto fragment_checksum = network::compute_internet_checksum(
        std::span<const std::uint8_t>{malformed}.first(
            Ipv4Packet::header_size));
    malformed[10] = static_cast<std::uint8_t>(fragment_checksum >> 8U);
    malformed[11] = static_cast<std::uint8_t>(fragment_checksum);
    suite.expect_false(Ipv4Packet::parse(malformed).has_value(),
                       "reject fragmented IPv4 packet");

    const std::vector<std::uint8_t> truncated(19U, 0U);
    suite.expect_false(Ipv4Packet::parse(truncated).has_value(),
                       "reject truncated IPv4 header");
}

}  // namespace silicon_switch::test
