#include "routing/ipv4_forwarding_engine_test.hpp"

#include "silicon_switch/network/ip_protocol.hpp"
#include "silicon_switch/network/ipv4_address.hpp"
#include "silicon_switch/network/ipv4_packet.hpp"
#include "silicon_switch/network/ipv4_prefix.hpp"
#include "silicon_switch/routing/ipv4_forwarding_engine.hpp"
#include "silicon_switch/routing/ipv4_forwarding_result.hpp"
#include "silicon_switch/routing/ipv4_route_table.hpp"
#include "silicon_switch/routing/port_id.hpp"
#include "silicon_switch/routing/route_entry.hpp"
#include "test_support.hpp"

#include <cstdint>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

namespace silicon_switch::test {
namespace {

network::Ipv4Address address(const std::uint32_t value) {
    return network::Ipv4Address{value};
}

network::Ipv4Prefix prefix(
    const std::uint32_t value,
    const network::Ipv4Prefix::Length length) {
    return *network::Ipv4Prefix::create(address(value), length);
}

routing::PortId port(const std::uint16_t value) {
    return *routing::PortId::create(value);
}

routing::RouteEntry route(
    const network::Ipv4Prefix route_prefix,
    const std::optional<network::Ipv4Address> next_hop,
    const routing::PortId output_port) {
    return *routing::RouteEntry::create(route_prefix, next_hop, output_port);
}

network::Ipv4Packet packet_to(
    const network::Ipv4Address destination,
    const std::uint8_t time_to_live = 64U) {
    return *network::Ipv4Packet::create(
        address(0xC0A8010AU),
        destination,
        network::IpProtocol::udp,
        std::vector<std::uint8_t>{0x10U, 0x20U},
        time_to_live,
        0x1234U);
}

const routing::ForwardedIpv4Packet* forwarded(
    const routing::Ipv4ForwardingResult& result) {
    return std::get_if<routing::ForwardedIpv4Packet>(&result);
}

const routing::DroppedIpv4Packet* dropped(
    const routing::Ipv4ForwardingResult& result) {
    return std::get_if<routing::DroppedIpv4Packet>(&result);
}

}  // namespace

void run_ipv4_forwarding_engine_tests(TestSuite& suite) {
    routing::Ipv4RouteTable routes;
    suite.expect_equal(
        routes.add_or_replace(route(prefix(0U, 0U), address(0x0A000001U), port(1U))),
        routing::RouteUpdate::inserted,
        "configure forwarding default route");
    suite.expect_equal(
        routes.add_or_replace(route(
            prefix(0xCB007100U, 24U), address(0x0A000002U), port(2U))),
        routing::RouteUpdate::inserted,
        "configure forwarding subnet route");
    suite.expect_equal(
        routes.add_or_replace(route(
            prefix(0xCB007109U, 32U), std::nullopt, port(3U))),
        routing::RouteUpdate::inserted,
        "configure forwarding host route");

    const routing::Ipv4ForwardingEngine engine{std::move(routes)};
    suite.expect_equal(engine.route_table().size(), std::size_t{3U},
                       "forwarding engine owns route table");

    const auto default_packet = packet_to(address(0x08080808U));
    const auto default_result = engine.forward(default_packet);
    const auto* default_forward = forwarded(default_result);
    suite.expect_true(default_forward != nullptr,
                      "forward packet through default route");
    if (default_forward != nullptr) {
        suite.expect_equal(default_forward->output_port(), port(1U),
                           "select default-route output port");
        suite.expect_equal(default_forward->next_hop(), address(0x0A000001U),
                           "select default-route gateway");
    }

    const auto subnet_result = engine.forward(packet_to(address(0xCB00712AU)));
    const auto* subnet_forward = forwarded(subnet_result);
    suite.expect_true(subnet_forward != nullptr,
                      "forward packet through subnet route");
    if (subnet_forward != nullptr) {
        suite.expect_equal(subnet_forward->output_port(), port(2U),
                           "prefer subnet route over default route");
        suite.expect_equal(subnet_forward->next_hop(), address(0x0A000002U),
                           "select subnet-route gateway");
    }

    const auto host_destination = address(0xCB007109U);
    const auto host_packet = packet_to(host_destination);
    const auto host_result = engine.forward(host_packet);
    const auto* host_forward = forwarded(host_result);
    suite.expect_true(host_forward != nullptr,
                      "forward packet through direct host route");
    if (host_forward != nullptr) {
        suite.expect_equal(host_forward->output_port(), port(3U),
                           "prefer host route over subnet route");
        suite.expect_equal(host_forward->next_hop(), host_destination,
                           "use destination as direct-route next hop");
        suite.expect_equal(host_forward->packet().time_to_live(),
                           std::uint8_t{63U},
                           "forward TTL-decremented packet");
    }
    suite.expect_equal(host_packet.time_to_live(), std::uint8_t{64U},
                       "forwarding engine preserves input packet");

    const auto ttl_result = engine.forward(packet_to(host_destination, 1U));
    const auto* ttl_drop = dropped(ttl_result);
    suite.expect_true(ttl_drop != nullptr, "drop expired packet in forwarding engine");
    if (ttl_drop != nullptr) {
        suite.expect_equal(ttl_drop->reason(),
                           routing::Ipv4DropReason::time_to_live_expired,
                           "forwarding engine reports TTL expiration");
    }

    const routing::Ipv4ForwardingEngine empty_engine{routing::Ipv4RouteTable{}};
    const auto missing_result = empty_engine.forward(packet_to(host_destination));
    const auto* route_drop = dropped(missing_result);
    suite.expect_true(route_drop != nullptr,
                      "drop packet without matching route");
    if (route_drop != nullptr) {
        suite.expect_equal(route_drop->reason(),
                           routing::Ipv4DropReason::route_not_found,
                           "forwarding engine reports route miss");
    }
}

}  // namespace silicon_switch::test
