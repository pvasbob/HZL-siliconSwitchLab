#include "network/ipv4_prefix_test.hpp"

#include "silicon_switch/network/ipv4_address.hpp"
#include "silicon_switch/network/ipv4_prefix.hpp"
#include "test_support.hpp"

#include <array>
#include <string>
#include <string_view>

namespace silicon_switch::test {

void run_ipv4_prefix_tests(TestSuite& suite) {
    using network::Ipv4Address;
    using network::Ipv4Prefix;

    const auto prefix = Ipv4Prefix::parse("192.168.1.123/24");
    suite.expect_true(prefix.has_value(), "parse IPv4 prefix");
    if (prefix.has_value()) {
        suite.expect_equal(
            prefix->network_address(),
            Ipv4Address{0xC0A80100U},
            "normalize IPv4 prefix network address");
        suite.expect_equal(
            prefix->length(), Ipv4Prefix::Length{24U}, "store prefix length");
        suite.expect_equal(
            prefix->subnet_mask(),
            Ipv4Address{0xFFFFFF00U},
            "calculate IPv4 subnet mask");
        suite.expect_equal(
            prefix->last_address(),
            Ipv4Address{0xC0A801FFU},
            "calculate final address in IPv4 prefix");
        suite.expect_equal(
            prefix->to_string(),
            std::string{"192.168.1.0/24"},
            "format normalized IPv4 prefix");
        suite.expect_true(
            prefix->contains(Ipv4Address{0xC0A8012AU}),
            "IPv4 prefix contains address in subnet");
        suite.expect_false(
            prefix->contains(Ipv4Address{0xC0A8022AU}),
            "IPv4 prefix excludes address outside subnet");
    }

    constexpr auto default_route =
        Ipv4Prefix::create(Ipv4Address{0xCB007109U}, 0U);
    static_assert(default_route.has_value());
    static_assert(default_route->network_address().value() == 0U);
    static_assert(default_route->subnet_mask().value() == 0U);
    static_assert(default_route->last_address().value() == 0xFFFFFFFFU);
    static_assert(default_route->contains(Ipv4Address{0xFFFFFFFFU}));
    suite.expect_equal(
        default_route->to_string(),
        std::string{"0.0.0.0/0"},
        "normalize and format default route");

    constexpr auto host_route =
        Ipv4Prefix::create(Ipv4Address{0xC0000207U}, 32U);
    static_assert(host_route.has_value());
    static_assert(host_route->network_address().value() == 0xC0000207U);
    static_assert(host_route->subnet_mask().value() == 0xFFFFFFFFU);
    static_assert(host_route->last_address().value() == 0xC0000207U);
    static_assert(host_route->contains(Ipv4Address{0xC0000207U}));
    static_assert(!host_route->contains(Ipv4Address{0xC0000208U}));
    suite.expect_equal(
        host_route->to_string(),
        std::string{"192.0.2.7/32"},
        "format IPv4 host route");

    constexpr auto invalid_length =
        Ipv4Prefix::create(Ipv4Address{0xC0000207U}, 33U);
    static_assert(!invalid_length.has_value());
    suite.expect_false(
        invalid_length.has_value(), "reject invalid numeric prefix length");

    constexpr std::array<std::string_view, 12> invalid_prefixes{
        "",
        "192.168.1.0",
        "/24",
        "192.168.1.0/",
        "192.168.1.0/-1",
        "192.168.1.0/33",
        "192.168.1.0/024",
        "192.168.1.0/24x",
        "192.168.1.0/24/1",
        "192.168.1.256/24",
        "192.168.1.0 /24",
        "192.168.1.0/ 24",
    };

    for (const auto text : invalid_prefixes) {
        suite.expect_false(
            Ipv4Prefix::parse(text).has_value(),
            "reject malformed IPv4 prefix");
    }

    constexpr auto shorter =
        Ipv4Prefix::create(Ipv4Address{0x0A000000U}, 8U);
    constexpr auto longer =
        Ipv4Prefix::create(Ipv4Address{0x0A000000U}, 24U);
    static_assert(shorter.has_value() && longer.has_value());
    suite.expect_true(*shorter < *longer, "order IPv4 prefixes");
    suite.expect_true(*shorter != *longer, "compare unequal IPv4 prefixes");
}

}  // namespace silicon_switch::test
