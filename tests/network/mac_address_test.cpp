#include "network/mac_address_test.hpp"

#include "silicon_switch/network/mac_address.hpp"
#include "test_support.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace silicon_switch::test {

void run_mac_address_tests(TestSuite& suite) {
    using network::MacAddress;

    const auto uppercase = MacAddress::parse("00:1A:2B:3C:4D:5E");
    suite.expect_true(uppercase.has_value(), "parse uppercase MAC address");

    if (uppercase.has_value()) {
        constexpr MacAddress::Bytes expected_bytes{
            0x00U, 0x1AU, 0x2BU, 0x3CU, 0x4DU, 0x5EU,
        };
        suite.expect_equal(
            uppercase->bytes(), expected_bytes, "parsed MAC address bytes");
        suite.expect_equal(
            uppercase->to_string(),
            std::string{"00:1A:2B:3C:4D:5E"},
            "format MAC address");
    }

    const auto lowercase = MacAddress::parse("aa:bb:cc:dd:ee:f0");
    suite.expect_true(lowercase.has_value(), "parse lowercase MAC address");
    if (lowercase.has_value()) {
        suite.expect_equal(
            lowercase->to_string(),
            std::string{"AA:BB:CC:DD:EE:F0"},
            "format lowercase input canonically");
    }

    constexpr std::array<std::string_view, 8> invalid_addresses{
        "",
        "00:11:22:33:44",
        "00:11:22:33:44:55:66",
        "0:11:22:33:44:55",
        "00-11-22-33-44-55",
        "00:11:22:33:44:GG",
        "00:11:22:33::44:55",
        "00:11:22:33:4455",
    };

    for (const auto text : invalid_addresses) {
        suite.expect_false(
            MacAddress::parse(text).has_value(), "reject malformed MAC address");
    }

    constexpr MacAddress broadcast{
        MacAddress::Bytes{0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU}};
    suite.expect_true(broadcast.is_broadcast(), "detect broadcast MAC address");
    suite.expect_true(
        broadcast.is_multicast(), "broadcast has the multicast/group bit set");
    suite.expect_false(
        broadcast.is_unicast(), "broadcast is not a unicast MAC address");

    constexpr MacAddress multicast{
        MacAddress::Bytes{0x01U, 0x00U, 0x5EU, 0x00U, 0x00U, 0x01U}};
    suite.expect_false(
        multicast.is_broadcast(), "multicast address is not broadcast");
    suite.expect_true(multicast.is_multicast(), "detect multicast MAC address");
    suite.expect_false(
        multicast.is_unicast(), "multicast address is not unicast");

    constexpr MacAddress unicast{
        MacAddress::Bytes{0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U}};
    suite.expect_false(unicast.is_broadcast(), "unicast is not broadcast");
    suite.expect_false(unicast.is_multicast(), "unicast is not multicast");
    suite.expect_true(unicast.is_unicast(), "detect unicast MAC address");

    constexpr MacAddress smaller{
        MacAddress::Bytes{0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U}};
    constexpr MacAddress larger{
        MacAddress::Bytes{0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U}};
    suite.expect_true(smaller < larger, "order MAC addresses lexicographically");
    suite.expect_true(smaller == smaller, "compare equal MAC addresses");
    suite.expect_true(smaller != larger, "compare unequal MAC addresses");
}

}  // namespace silicon_switch::test
