#include "network/ipv4_address_test.hpp"

#include "silicon_switch/network/ipv4_address.hpp"
#include "test_support.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace silicon_switch::test {

void run_ipv4_address_tests(TestSuite& suite) {
    using network::Ipv4Address;

    const auto typical = Ipv4Address::parse("192.168.1.10");
    suite.expect_true(typical.has_value(), "parse typical IPv4 address");
    if (typical.has_value()) {
        constexpr Ipv4Address::Octets expected_octets{
            192U, 168U, 1U, 10U};
        suite.expect_equal(
            typical->octets(), expected_octets, "parsed IPv4 octets");
        suite.expect_equal(
            typical->value(), 0xC0A8010AU, "packed IPv4 numeric value");
        suite.expect_equal(
            typical->to_string(),
            std::string{"192.168.1.10"},
            "format IPv4 address");
    }

    const auto minimum = Ipv4Address::parse("0.0.0.0");
    suite.expect_true(minimum.has_value(), "parse minimum IPv4 address");
    if (minimum.has_value()) {
        suite.expect_true(
            minimum->is_unspecified(), "detect unspecified IPv4 address");
    }

    const auto maximum = Ipv4Address::parse("255.255.255.255");
    suite.expect_true(maximum.has_value(), "parse maximum IPv4 address");
    if (maximum.has_value()) {
        suite.expect_true(
            maximum->is_limited_broadcast(),
            "detect limited-broadcast IPv4 address");
    }

    constexpr std::array<std::string_view, 13> invalid_addresses{
        "",
        "192.168.1",
        "192.168.1.10.20",
        ".168.1.10",
        "192..1.10",
        "192.168.1.",
        "192.168.1.256",
        "192.168.-1.10",
        "192.168.1a.10",
        "192:168:1:10",
        "01.2.3.4",
        "1.002.3.4",
        "1234.2.3.4",
    };

    for (const auto text : invalid_addresses) {
        suite.expect_false(
            Ipv4Address::parse(text).has_value(),
            "reject malformed IPv4 address");
    }

    constexpr Ipv4Address loopback{
        Ipv4Address::Octets{127U, 0U, 0U, 1U}};
    suite.expect_true(loopback.is_loopback(), "detect IPv4 loopback address");
    suite.expect_false(
        loopback.is_multicast(), "loopback is not IPv4 multicast");

    constexpr Ipv4Address multicast{
        Ipv4Address::Octets{239U, 1U, 2U, 3U}};
    suite.expect_true(
        multicast.is_multicast(), "detect IPv4 multicast address");
    suite.expect_false(
        multicast.is_loopback(), "multicast is not IPv4 loopback");

    constexpr Ipv4Address ordinary{
        Ipv4Address::Octets{192U, 168U, 1U, 1U}};
    suite.expect_false(
        ordinary.is_unspecified(), "ordinary IPv4 is not unspecified");
    suite.expect_false(
        ordinary.is_loopback(), "ordinary IPv4 is not loopback");
    suite.expect_false(
        ordinary.is_multicast(), "ordinary IPv4 is not multicast");
    suite.expect_false(
        ordinary.is_limited_broadcast(),
        "ordinary IPv4 is not limited broadcast");

    constexpr Ipv4Address smaller{
        Ipv4Address::Octets{10U, 0U, 0U, 1U}};
    constexpr Ipv4Address larger{
        Ipv4Address::Octets{10U, 0U, 0U, 2U}};
    suite.expect_true(smaller < larger, "order IPv4 addresses numerically");
    suite.expect_true(smaller == smaller, "compare equal IPv4 addresses");
    suite.expect_true(smaller != larger, "compare unequal IPv4 addresses");

    constexpr Ipv4Address from_value{0xC0A8010AU};
    suite.expect_equal(
        from_value.octets(),
        Ipv4Address::Octets{192U, 168U, 1U, 10U},
        "extract IPv4 octets from numeric value");
}

}  // namespace silicon_switch::test
