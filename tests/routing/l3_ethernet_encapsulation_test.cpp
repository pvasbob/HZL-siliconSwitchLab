#include "routing/l3_ethernet_encapsulation_test.hpp"

#include "silicon_switch/network/ether_type.hpp"
#include "silicon_switch/network/ethernet_frame.hpp"
#include "silicon_switch/network/ip_protocol.hpp"
#include "silicon_switch/network/ipv4_address.hpp"
#include "silicon_switch/network/ipv4_packet.hpp"
#include "silicon_switch/network/mac_address.hpp"
#include "silicon_switch/routing/arp_cache.hpp"
#include "silicon_switch/routing/ipv4_forwarding_result.hpp"
#include "silicon_switch/routing/l3_ethernet_encapsulation.hpp"
#include "silicon_switch/routing/port_id.hpp"
#include "test_support.hpp"

#include <chrono>
#include <cstdint>
#include <utility>
#include <variant>
#include <vector>

namespace silicon_switch::test {
namespace {

network::MacAddress mac(const network::MacAddress::Bytes bytes) {
    return network::MacAddress{bytes};
}

network::Ipv4Address ip(const std::uint32_t value) {
    return network::Ipv4Address{value};
}

network::Ipv4Packet packet_to(
    const network::Ipv4Address destination,
    network::Ipv4Packet::Payload payload = {0x12U, 0x34U}) {
    return *network::Ipv4Packet::create(
        ip(0xC0A8010AU),
        destination,
        network::IpProtocol::udp,
        std::move(payload),
        63U,
        0x4567U);
}

routing::ForwardedIpv4Packet forwarding(
    network::Ipv4Packet packet,
    const network::Ipv4Address next_hop) {
    return routing::ForwardedIpv4Packet{
        std::move(packet), *routing::PortId::create(7U), next_hop};
}

}  // namespace

void run_l3_ethernet_encapsulation_tests(TestSuite& suite) {
    using namespace std::chrono_literals;

    const auto router_mac =
        mac({0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U});
    const auto neighbor_mac =
        mac({0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U});
    const auto destination = ip(0xCB007109U);
    const routing::ArpCache::TimePoint start{};
    routing::ArpCache cache{30s};
    suite.expect_equal(
        cache.add_or_replace(destination, neighbor_mac, start),
        routing::ArpCacheUpdate::inserted,
        "learn direct-route ARP neighbor");

    const auto direct_forwarding = forwarding(packet_to(destination), destination);
    const auto original_packet_bytes = direct_forwarding.packet().serialize();
    const auto direct_result = routing::encapsulate_ipv4_in_ethernet(
        direct_forwarding, router_mac, cache, start + 1s);
    const auto* direct_frame =
        std::get_if<network::EthernetFrame>(&direct_result);
    suite.expect_true(direct_frame != nullptr,
                      "encapsulate directly routed IPv4 packet");
    if (direct_frame != nullptr) {
        suite.expect_equal(direct_frame->source(), router_mac,
                           "use router egress source MAC");
        suite.expect_equal(direct_frame->destination(), neighbor_mac,
                           "use resolved direct-neighbor destination MAC");
        suite.expect_equal(direct_frame->ether_type(), network::EtherType::ipv4,
                           "set IPv4 Ethernet type");
        suite.expect_false(direct_frame->vlan_tag().has_value(),
                           "create untagged Layer 3 Ethernet frame");
        suite.expect_equal(direct_frame->payload(), original_packet_bytes,
                           "encapsulate forwarded IPv4 serialization");

        const auto parsed_frame =
            network::EthernetFrame::parse(direct_frame->serialize());
        suite.expect_true(parsed_frame.has_value(),
                          "round trip Layer 3 Ethernet frame");
        if (parsed_frame.has_value()) {
            const auto parsed_packet =
                network::Ipv4Packet::parse(parsed_frame->payload());
            suite.expect_true(parsed_packet.has_value(),
                              "parse encapsulated IPv4 packet");
            if (parsed_packet.has_value()) {
                suite.expect_equal(*parsed_packet, direct_forwarding.packet(),
                                   "preserve forwarded packet through encapsulation");
            }
        }
    }
    suite.expect_equal(direct_forwarding.packet().serialize(), original_packet_bytes,
                       "encapsulation preserves forwarding result");

    const auto gateway = ip(0xC0A80101U);
    const auto gateway_mac =
        mac({0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x03U});
    suite.expect_equal(
        cache.add_or_replace(gateway, gateway_mac, start),
        routing::ArpCacheUpdate::inserted,
        "learn gateway ARP neighbor");
    const auto gateway_result = routing::encapsulate_ipv4_in_ethernet(
        forwarding(packet_to(destination), gateway),
        router_mac,
        cache,
        start + 1s);
    const auto* gateway_frame =
        std::get_if<network::EthernetFrame>(&gateway_result);
    suite.expect_true(gateway_frame != nullptr,
                      "encapsulate gateway-routed IPv4 packet");
    if (gateway_frame != nullptr) {
        suite.expect_equal(gateway_frame->destination(), gateway_mac,
                           "resolve gateway rather than packet destination");
    }

    const auto missing_result = routing::encapsulate_ipv4_in_ethernet(
        forwarding(packet_to(destination), ip(0xC0A801FEU)),
        router_mac,
        cache,
        start + 1s);
    suite.expect_equal(
        std::get<routing::L3EncapsulationFailure>(missing_result),
        routing::L3EncapsulationFailure::neighbor_not_found,
        "report unresolved next-hop neighbor");

    const auto expired_result = routing::encapsulate_ipv4_in_ethernet(
        direct_forwarding, router_mac, cache, start + 30s);
    suite.expect_equal(
        std::get<routing::L3EncapsulationFailure>(expired_result),
        routing::L3EncapsulationFailure::neighbor_not_found,
        "reject expired ARP neighbor during encapsulation");

    const auto invalid_source_result = routing::encapsulate_ipv4_in_ethernet(
        direct_forwarding,
        mac({0U, 0U, 0U, 0U, 0U, 0U}),
        cache,
        start + 1s);
    suite.expect_equal(
        std::get<routing::L3EncapsulationFailure>(invalid_source_result),
        routing::L3EncapsulationFailure::invalid_source_mac,
        "reject unspecified router source MAC");

    auto oversized_payload = network::Ipv4Packet::Payload(
        network::EthernetFrame::maximum_payload_size -
            network::Ipv4Packet::header_size + 1U,
        0xABU);
    const auto oversized_result = routing::encapsulate_ipv4_in_ethernet(
        forwarding(packet_to(destination, std::move(oversized_payload)), destination),
        router_mac,
        cache,
        start + 1s);
    suite.expect_equal(
        std::get<routing::L3EncapsulationFailure>(oversized_result),
        routing::L3EncapsulationFailure::packet_too_large,
        "reject IPv4 packet exceeding Ethernet payload limit");

    routing::ArpCache realtime_cache;
    suite.expect_equal(
        realtime_cache.add_or_replace(destination, neighbor_mac),
        routing::ArpCacheUpdate::inserted,
        "learn neighbor using current monotonic time");
    suite.expect_true(
        std::holds_alternative<network::EthernetFrame>(
            routing::encapsulate_ipv4_in_ethernet(
                direct_forwarding, router_mac, realtime_cache)),
        "encapsulate using production-time overload");
}

}  // namespace silicon_switch::test
