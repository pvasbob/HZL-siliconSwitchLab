#include "silicon_switch/network/mac_address.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace silicon_switch::network {
namespace {

[[nodiscard]] constexpr std::optional<std::uint8_t>
hex_digit_value(const char character) noexcept {
    if (character >= '0' && character <= '9') {
        return static_cast<std::uint8_t>(character - '0');
    }

    if (character >= 'a' && character <= 'f') {
        return static_cast<std::uint8_t>(character - 'a' + 10);
    }

    if (character >= 'A' && character <= 'F') {
        return static_cast<std::uint8_t>(character - 'A' + 10);
    }

    return std::nullopt;
}

}  // namespace  // That gives it internal linkage. 
                 // It can only be used inside this .cpp translation unit.

std::optional<MacAddress> MacAddress::parse(
    const std::string_view text) noexcept {
    constexpr std::size_t characters_per_byte = 2;
    constexpr std::size_t separator_count = byte_count - 1;
    constexpr std::size_t expected_length =
        (byte_count * characters_per_byte) + separator_count;

    if (text.size() != expected_length) {
        return std::nullopt;
    }

    Bytes bytes{};

    for (std::size_t index = 0; index < byte_count; ++index) {
        const std::size_t character_index = index * 3;

        const auto high_nibble = hex_digit_value(text[character_index]);
        const auto low_nibble = hex_digit_value(text[character_index + 1]);

        if (!high_nibble.has_value() || !low_nibble.has_value()) {
            return std::nullopt;
        }

        if (index + 1 < byte_count && text[character_index + 2] != ':') {
            return std::nullopt;
        }

        bytes[index] = static_cast<std::uint8_t>(
            (static_cast<std::uint16_t>(*high_nibble) << 4U) |
            static_cast<std::uint16_t>(*low_nibble));
    }

    return MacAddress{bytes};
}

std::string MacAddress::to_string() const {
    constexpr std::array<char, 16> hexadecimal_digits{
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
    };
    constexpr std::size_t output_length = 17;

    std::string result(output_length, ':');

    for (std::size_t index = 0; index < byte_count; ++index) {
        const std::size_t character_index = index * 3;
        const auto byte = bytes_[index];

        result[character_index] =
            hexadecimal_digits[static_cast<std::size_t>(byte >> 4U)];
        result[character_index + 1] =
            hexadecimal_digits[static_cast<std::size_t>(byte & 0x0FU)];
    }

    return result;
}

}  // namespace silicon_switch::network
