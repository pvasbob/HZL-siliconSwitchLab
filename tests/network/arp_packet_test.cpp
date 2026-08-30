#include "network/arp_packet_test.hpp"

#include "silicon_switch/network/arp_packet.hpp"
#include "silicon_switch/network/ipv4_address.hpp"
#include "silicon_switch/network/mac_address.hpp"
#include "test_support.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace silicon_switch::test {
namespace {

network::MacAddress mac(const network::MacAddress::Bytes bytes) {
    return network::MacAddress{bytes};
}

network::Ipv4Address ip(const std::uint32_t value) {
    return network::Ipv4Address{value};
}

}  // namespace

void run_arp_packet_tests(TestSuite& suite) {
    const auto sender_mac =
        mac({0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U});
    const auto target_mac =
        mac({0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U});
    const auto sender_ip = ip(0xC0A80101U);
    const auto target_ip = ip(0xC0A80102U);
    const auto zero_mac = mac({0U, 0U, 0U, 0U, 0U, 0U});

    const auto request = network::ArpPacket::create_request(
        sender_mac, sender_ip, target_ip);
    suite.expect_true(request.has_value(), "create Ethernet IPv4 ARP request");
    if (request.has_value()) {
        suite.expect_true(request->is_request(), "classify ARP request");
        suite.expect_false(request->is_reply(), "do not classify request as reply");
        suite.expect_equal(request->operation(), network::ArpOperation::request,
                           "store ARP request operation");
        suite.expect_equal(request->sender_mac(), sender_mac,
                           "store ARP request sender MAC");
        suite.expect_equal(request->sender_ip(), sender_ip,
                           "store ARP request sender IPv4 address");
        suite.expect_equal(request->target_mac(), zero_mac,
                           "use unspecified target MAC in ARP request");
        suite.expect_equal(request->target_ip(), target_ip,
                           "store ARP request target IPv4 address");

        const std::vector<std::uint8_t> expected{
            0x00U, 0x01U, 0x08U, 0x00U, 0x06U, 0x04U, 0x00U, 0x01U,
            0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U,
            0xC0U, 0xA8U, 0x01U, 0x01U,
            0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
            0xC0U, 0xA8U, 0x01U, 0x02U};
        const auto serialized = request->serialize();
        suite.expect_equal(serialized, expected,
                           "serialize ARP request in network byte order");
        const auto parsed = network::ArpPacket::parse(serialized);
        suite.expect_true(parsed.has_value(), "parse serialized ARP request");
        if (parsed.has_value()) {
            suite.expect_equal(*parsed, *request, "round trip ARP request");
        }
    }

    const auto reply = network::ArpPacket::create_reply(
        sender_mac, sender_ip, target_mac, target_ip);
    suite.expect_true(reply.has_value(), "create Ethernet IPv4 ARP reply");
    if (reply.has_value()) {
        suite.expect_true(reply->is_reply(), "classify ARP reply");
        suite.expect_equal(reply->target_mac(), target_mac,
                           "store ARP reply target MAC");
        const auto serialized = reply->serialize();
        suite.expect_equal(serialized.size(), network::ArpPacket::serialized_size,
                           "serialize fixed-size ARP reply");
        suite.expect_equal(serialized[7U], std::uint8_t{2U},
                           "serialize ARP reply operation");
        const auto parsed = network::ArpPacket::parse(serialized);
        suite.expect_true(parsed.has_value(), "parse serialized ARP reply");
        if (parsed.has_value()) {
            suite.expect_equal(*parsed, *reply, "round trip ARP reply");
        }
    }

    suite.expect_true(
        network::ArpPacket::create_request(sender_mac, ip(0U), target_ip)
            .has_value(),
        "allow unspecified sender IP for ARP probing");
    suite.expect_false(
        network::ArpPacket::create_request(zero_mac, sender_ip, target_ip)
            .has_value(),
        "reject unspecified ARP sender MAC");
    suite.expect_false(
        network::ArpPacket::create_reply(
            sender_mac, sender_ip, zero_mac, target_ip)
            .has_value(),
        "reject unspecified ARP reply target MAC");
    suite.expect_false(
        network::ArpPacket::create_reply(
            mac({0x01U, 0U, 0U, 0U, 0U, 1U}),
            sender_ip,
            target_mac,
            target_ip)
            .has_value(),
        "reject multicast ARP sender MAC");

    const auto valid_bytes = request->serialize();
    suite.expect_false(
        network::ArpPacket::parse(
            std::vector<std::uint8_t>(valid_bytes.begin(), valid_bytes.end() - 1))
            .has_value(),
        "reject truncated ARP packet");
    auto trailing_bytes = valid_bytes;
    trailing_bytes.push_back(0U);
    suite.expect_false(network::ArpPacket::parse(trailing_bytes).has_value(),
                       "reject trailing ARP bytes");

    auto unsupported_hardware = valid_bytes;
    unsupported_hardware[1U] = 2U;
    suite.expect_false(
        network::ArpPacket::parse(unsupported_hardware).has_value(),
        "reject unsupported ARP hardware type");
    auto unsupported_protocol = valid_bytes;
    unsupported_protocol[3U] = 6U;
    suite.expect_false(
        network::ArpPacket::parse(unsupported_protocol).has_value(),
        "reject unsupported ARP protocol type");
    auto invalid_hardware_length = valid_bytes;
    invalid_hardware_length[4U] = 5U;
    suite.expect_false(
        network::ArpPacket::parse(invalid_hardware_length).has_value(),
        "reject invalid ARP hardware address length");
    auto invalid_protocol_length = valid_bytes;
    invalid_protocol_length[5U] = 16U;
    suite.expect_false(
        network::ArpPacket::parse(invalid_protocol_length).has_value(),
        "reject invalid ARP protocol address length");
    auto invalid_operation = valid_bytes;
    invalid_operation[7U] = 3U;
    suite.expect_false(network::ArpPacket::parse(invalid_operation).has_value(),
                       "reject unsupported ARP operation");
    auto request_with_target_mac = valid_bytes;
    request_with_target_mac[18U] = 0x02U;
    suite.expect_false(
        network::ArpPacket::parse(request_with_target_mac).has_value(),
        "reject ARP request with specified target MAC");
}

}  // namespace silicon_switch::test
