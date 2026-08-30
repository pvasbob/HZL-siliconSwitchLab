#include "routing/ipv4_route_table_test.hpp"

#include "silicon_switch/network/ipv4_address.hpp"
#include "silicon_switch/network/ipv4_prefix.hpp"
#include "silicon_switch/routing/ipv4_route_table.hpp"
#include "silicon_switch/routing/port_id.hpp"
#include "silicon_switch/routing/route_entry.hpp"
#include "test_support.hpp"

#include <cstdint>
#include <optional>

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

}  // namespace

void run_ipv4_route_table_tests(TestSuite& suite) {
    suite.expect_false(routing::PortId::create(0U).has_value(),
                       "reject reserved zero port identifier");
    suite.expect_equal(port(7U).value(), std::uint16_t{7U},
                       "create strongly typed port identifier");

    const auto direct_route = routing::RouteEntry::create(
        prefix(0xC0A80100U, 24U), std::nullopt, port(1U));
    suite.expect_true(direct_route.has_value(),
                      "create directly connected route");
    if (direct_route.has_value()) {
        suite.expect_true(direct_route->is_directly_connected(),
                          "identify directly connected route");
    }

    const auto gateway_route = routing::RouteEntry::create(
        prefix(0U, 0U), address(0xC0A80101U), port(2U));
    suite.expect_true(gateway_route.has_value(), "create next-hop route");
    if (gateway_route.has_value()) {
        suite.expect_false(gateway_route->is_directly_connected(),
                           "identify route through next hop");
    }

    suite.expect_false(
        routing::RouteEntry::create(
            prefix(0U, 0U), address(0U), port(1U))
            .has_value(),
        "reject unspecified route next hop");
    suite.expect_false(
        routing::RouteEntry::create(
            prefix(0U, 0U), address(0xE0000001U), port(1U))
            .has_value(),
        "reject multicast route next hop");
    suite.expect_false(
        routing::RouteEntry::create(
            prefix(0U, 0U), address(0xFFFFFFFFU), port(1U))
            .has_value(),
        "reject broadcast route next hop");

    routing::Ipv4RouteTable table;
    suite.expect_true(table.empty(), "start with empty IPv4 route table");
    suite.expect_false(
        table.longest_prefix_match(address(0x08080808U)).has_value(),
        "report no route in empty table");

    const auto default_route =
        route(prefix(0U, 0U), address(0x0A000001U), port(1U));
    const auto private_route =
        route(prefix(0xC0A80000U, 16U), address(0x0A000002U), port(2U));
    const auto subnet_route =
        route(prefix(0xC0A80100U, 24U), std::nullopt, port(3U));
    const auto host_route =
        route(prefix(0xC0A8012AU, 32U), address(0xC0A80163U), port(4U));

    suite.expect_equal(
        table.add_or_replace(default_route),
        routing::RouteUpdate::inserted,
        "insert IPv4 default route");
    suite.expect_equal(
        table.add_or_replace(private_route),
        routing::RouteUpdate::inserted,
        "insert IPv4 network route");
    suite.expect_equal(
        table.add_or_replace(subnet_route),
        routing::RouteUpdate::inserted,
        "insert more-specific IPv4 route");
    suite.expect_equal(
        table.add_or_replace(host_route),
        routing::RouteUpdate::inserted,
        "insert IPv4 host route");
    suite.expect_equal(table.size(), std::size_t{4U},
                       "count inserted IPv4 routes");

    const auto default_match =
        table.longest_prefix_match(address(0x08080808U));
    suite.expect_true(default_match.has_value(), "match IPv4 default route");
    if (default_match.has_value()) {
        suite.expect_equal(default_match->output_port(), port(1U),
                           "select default-route output port");
    }

    const auto subnet_match =
        table.longest_prefix_match(address(0xC0A80132U));
    suite.expect_true(subnet_match.has_value(),
                      "find longest matching IPv4 subnet");
    if (subnet_match.has_value()) {
        suite.expect_equal(subnet_match->prefix(), prefix(0xC0A80100U, 24U),
                           "prefer longest IPv4 prefix");
        suite.expect_equal(subnet_match->output_port(), port(3U),
                           "select longest-prefix output port");
    }

    const auto exact_host_match =
        table.longest_prefix_match(address(0xC0A8012AU));
    suite.expect_true(exact_host_match.has_value(), "match IPv4 host route");
    if (exact_host_match.has_value()) {
        suite.expect_equal(exact_host_match->prefix(),
                           prefix(0xC0A8012AU, 32U),
                           "prefer IPv4 host route over subnet");
    }

    const auto exact = table.find_exact(prefix(0xC0A80000U, 16U));
    suite.expect_true(exact.has_value(), "find exact IPv4 route prefix");
    suite.expect_false(table.find_exact(prefix(0xAC100000U, 12U)).has_value(),
                       "report missing exact IPv4 route");

    const auto replacement =
        route(prefix(0xC0A80100U, 24U), address(0x0A000009U), port(9U));
    suite.expect_equal(
        table.add_or_replace(replacement),
        routing::RouteUpdate::replaced,
        "replace existing IPv4 route");
    suite.expect_equal(table.size(), std::size_t{4U},
                       "route replacement preserves table size");
    const auto replaced = table.find_exact(prefix(0xC0A80100U, 24U));
    if (replaced.has_value()) {
        suite.expect_equal(replaced->output_port(), port(9U),
                           "store replacement route attributes");
    }

    suite.expect_true(table.remove(prefix(0xC0A80100U, 24U)),
                      "remove existing IPv4 route");
    suite.expect_false(table.remove(prefix(0xC0A80100U, 24U)),
                       "report removal of missing IPv4 route");
    suite.expect_equal(table.size(), std::size_t{3U},
                       "decrease size after route removal");

    const auto fallback =
        table.longest_prefix_match(address(0xC0A80132U));
    suite.expect_true(fallback.has_value(),
                      "fall back after removing specific route");
    if (fallback.has_value()) {
        suite.expect_equal(fallback->prefix(), prefix(0xC0A80000U, 16U),
                           "select next-longest IPv4 prefix");
    }
}

}  // namespace silicon_switch::test
