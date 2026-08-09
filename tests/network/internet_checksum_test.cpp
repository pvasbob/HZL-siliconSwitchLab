#include "network/internet_checksum_test.hpp"

#include "silicon_switch/network/internet_checksum.hpp"
#include "silicon_switch/network/ip_protocol.hpp"
#include "test_support.hpp"

#include <array>
#include <cstdint>
#include <type_traits>

namespace silicon_switch::test {

void run_internet_checksum_tests(TestSuite& suite) {
    using network::compute_internet_checksum;
    using network::has_valid_internet_checksum;
    using network::IpProtocol;

    static_assert(std::is_same_v<
                  std::underlying_type_t<IpProtocol>,
                  std::uint8_t>);
    static_assert(static_cast<std::uint8_t>(IpProtocol::icmp) == 1U);
    static_assert(static_cast<std::uint8_t>(IpProtocol::tcp) == 6U);
    static_assert(static_cast<std::uint8_t>(IpProtocol::udp) == 17U);
    suite.expect_true(true, "define standard IPv4 protocol identifiers");

    // Based on the template argument, each array element is a std::uint8_t, 
    // an unsigned integer type with exactly 8 bits. The initializer literals 
    // are normally unsigned int, but they are converted to std::uint8_t when stored.
    constexpr std::array<std::uint8_t, 20> header_without_checksum{
        0x45U, 0x00U, 0x00U, 0x73U,
        0x00U, 0x00U, 0x40U, 0x00U,
        0x40U, 0x11U, 0x00U, 0x00U,
        0xC0U, 0xA8U, 0x00U, 0x01U,
        0xC0U, 0xA8U, 0x00U, 0xC7U,
    };
    suite.expect_equal(
        compute_internet_checksum(header_without_checksum),
        std::uint16_t{0xB861U},
        "compute known IPv4 header checksum");

    constexpr std::array<std::uint8_t, 20> header_with_checksum{
        0x45U, 0x00U, 0x00U, 0x73U,
        0x00U, 0x00U, 0x40U, 0x00U,
        0x40U, 0x11U, 0xB8U, 0x61U,
        0xC0U, 0xA8U, 0x00U, 0x01U,
        0xC0U, 0xA8U, 0x00U, 0xC7U,
    };
    suite.expect_true(
        has_valid_internet_checksum(header_with_checksum),
        "validate IPv4 header checksum");

    auto corrupted_header = header_with_checksum;
    corrupted_header[8] = 0x3FU;
    suite.expect_false(
        has_valid_internet_checksum(corrupted_header),
        "reject corrupted IPv4 header checksum");

    constexpr std::array<std::uint8_t, 3> odd_length_bytes{
        0x01U, 0x02U, 0x03U};
    suite.expect_equal(
        compute_internet_checksum(odd_length_bytes),
        std::uint16_t{0xFBFDU},
        "pad odd-length checksum input with zero byte");

    constexpr std::array<std::uint8_t, 0> empty_bytes{};
    suite.expect_equal(
        compute_internet_checksum(empty_bytes),
        std::uint16_t{0xFFFFU},
        "compute checksum of empty input");
    suite.expect_false(
        has_valid_internet_checksum(empty_bytes),
        "reject empty checksum input as invalid packet data");
}

}  // namespace silicon_switch::test
