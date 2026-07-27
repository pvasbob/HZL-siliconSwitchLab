#include "network/ethernet_frame_test.hpp"

#include "silicon_switch/network/ether_type.hpp"
#include "silicon_switch/network/ethernet_frame.hpp"
#include "silicon_switch/network/mac_address.hpp"
#include "test_support.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace silicon_switch::test {

void run_ethernet_frame_tests(TestSuite& suite) {
    using network::EthernetFrame;
    using network::EtherType;
    using network::MacAddress;

    const std::vector<std::uint8_t> wire_bytes{
        0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
        0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U,
        0x08U, 0x00U,
        0x45U, 0x00U, 0x00U, 0x14U,
    };

    const auto frame = EthernetFrame::parse(wire_bytes);
    suite.expect_true(frame.has_value(), "parse Ethernet frame");
    if (frame.has_value()) {
        suite.expect_true(
            frame->destination().is_broadcast(),
            "parse Ethernet destination MAC address");
        suite.expect_equal(
            frame->source(),
            MacAddress{MacAddress::Bytes{
                0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U}},
            "parse Ethernet source MAC address");
        suite.expect_equal(
            frame->ether_type(),
            EtherType::ipv4,
            "parse Ethernet type in network byte order");
        suite.expect_equal(
            frame->payload(),
            EthernetFrame::Payload{0x45U, 0x00U, 0x00U, 0x14U},
            "parse Ethernet payload");
        suite.expect_equal(
            frame->serialize(),
            wire_bytes,
            "round-trip Ethernet frame serialization");
    }

    constexpr std::array<std::uint8_t, EthernetFrame::header_size - 1U>
        truncated_frame{};
    suite.expect_false(
        EthernetFrame::parse(truncated_frame).has_value(),
        "reject truncated Ethernet header");

    std::vector<std::uint8_t> oversized_wire_frame(
        EthernetFrame::header_size + EthernetFrame::maximum_payload_size + 1U);
    suite.expect_false(
        EthernetFrame::parse(oversized_wire_frame).has_value(),
        "reject oversized Ethernet payload while parsing");

    const MacAddress destination{
        MacAddress::Bytes{0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U}};
    const MacAddress source{
        MacAddress::Bytes{0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U}};

    auto created = EthernetFrame::create(
        destination,
        source,
        EtherType::arp,
        EthernetFrame::Payload{0x00U, 0x01U});
    suite.expect_true(created.has_value(), "create valid Ethernet frame");
    if (created.has_value()) {
        suite.expect_equal(
            created->serialize(),
            std::vector<std::uint8_t>{
                0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U,
                0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U,
                0x08U, 0x06U,
                0x00U, 0x01U,
            },
            "serialize Ethernet frame in wire order");
    }

    EthernetFrame::Payload oversized_payload(
        EthernetFrame::maximum_payload_size + 1U);
    suite.expect_false(
        EthernetFrame::create(
            destination,
            source,
            EtherType::ipv4,
            std::move(oversized_payload))
            .has_value(),
        "reject oversized Ethernet payload while creating");

    const std::vector<std::uint8_t> unknown_type_bytes{
        0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U,
        0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U,
        0x88U, 0xB5U,
    };
    const auto unknown_type_frame = EthernetFrame::parse(unknown_type_bytes);
    suite.expect_true(
        unknown_type_frame.has_value(),
        "preserve unknown Ethernet type");
    if (unknown_type_frame.has_value()) {
        suite.expect_equal(
            static_cast<std::uint16_t>(unknown_type_frame->ether_type()),
            std::uint16_t{0x88B5U},
            "retain unknown Ethernet type numeric value");
    }
}

}  // namespace silicon_switch::test
