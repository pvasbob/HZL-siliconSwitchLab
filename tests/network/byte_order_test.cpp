#include "network/byte_order_test.hpp"

#include "silicon_switch/network/byte_order.hpp"
#include "test_support.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <span>

namespace silicon_switch::test {

void run_byte_order_tests(TestSuite& suite) {
    using network::wire::read_big_endian;
    using network::wire::write_big_endian;

    constexpr std::array<std::uint8_t, 8> bytes{
        0x08U, 0x00U, 0x12U, 0x34U, 0x56U, 0x78U, 0x9AU, 0xBCU};

    constexpr auto sixteen_bit_value =
        read_big_endian<std::uint16_t>(bytes);
    static_assert(sixteen_bit_value == std::uint16_t{0x0800U});
    suite.expect_equal(
        sixteen_bit_value,
        std::optional<std::uint16_t>{0x0800U},
        "read 16-bit big-endian value");

    constexpr auto thirty_two_bit_value =
        read_big_endian<std::uint32_t>(bytes, 2U);
    static_assert(thirty_two_bit_value == std::uint32_t{0x12345678U});
    suite.expect_equal(
        thirty_two_bit_value,
        std::optional<std::uint32_t>{0x12345678U},
        "read 32-bit big-endian value at offset");

    constexpr auto sixty_four_bit_value =
        read_big_endian<std::uint64_t>(bytes);
    static_assert(
        sixty_four_bit_value == std::uint64_t{0x0800123456789ABCU});
    suite.expect_equal(
        sixty_four_bit_value,
        std::optional<std::uint64_t>{0x0800123456789ABCU},
        "read 64-bit big-endian value");

    suite.expect_false(
        read_big_endian<std::uint32_t>(bytes, 6U).has_value(),
        "reject truncated big-endian read");
    suite.expect_false(
        read_big_endian<std::uint16_t>(bytes, bytes.size() + 1U).has_value(),
        "reject big-endian read offset beyond buffer");

    std::array<std::uint8_t, 8> output{};
    suite.expect_true(
        write_big_endian<std::uint16_t>(0x0800U, output, 1U),
        "write 16-bit big-endian value");
    suite.expect_true(
        output[1] == 0x08U && output[2] == 0x00U,
        "store 16-bit value in network byte order");

    suite.expect_true(
        write_big_endian<std::uint32_t>(0x12345678U, output, 3U),
        "write 32-bit big-endian value");
    suite.expect_equal(
        read_big_endian<std::uint32_t>(output, 3U),
        std::optional<std::uint32_t>{0x12345678U},
        "round-trip 32-bit big-endian value");

    const auto before_failed_write = output;
    suite.expect_false(
        write_big_endian<std::uint32_t>(0xFFFFFFFFU, output, 6U),
        "reject truncated big-endian write");
    suite.expect_equal(
        output,
        before_failed_write,
        "failed big-endian write leaves buffer unchanged");

    const std::span<std::uint8_t> empty_output{};
    suite.expect_false(
        write_big_endian<std::uint16_t>(0x0800U, empty_output),
        "reject write to empty buffer");
}

}  // namespace silicon_switch::test
