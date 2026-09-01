#include "silicon_switch/network/ipv4_address.hpp"

#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace silicon_switch::network {

std::optional<Ipv4Address> Ipv4Address::parse(
    const std::string_view text) noexcept {
    Octets octets{};
    std::size_t position = 0;

    for (std::size_t octet_index = 0; octet_index < octet_count; ++octet_index) {
        if (position >= text.size()) {
            return std::nullopt;
        }

        const std::size_t octet_start = position;
        std::uint16_t octet_value = 0;
        std::size_t digit_count = 0;

        while (position < text.size() && text[position] != '.') {
            const char character = text[position];
            if (character < '0' || character > '9') {
                return std::nullopt;
            }

            if (digit_count == 3) {
                return std::nullopt;
            }

            octet_value = static_cast<std::uint16_t>(
                (octet_value * 10U) +
                static_cast<std::uint16_t>(character - '0'));
            if (octet_value > 255U) {
                return std::nullopt;
            }

            ++digit_count;
            ++position;
        }

        if (digit_count == 0) {
            return std::nullopt;
        }

        if (digit_count > 1 && text[octet_start] == '0') {
            return std::nullopt;
        }

        octets[octet_index] = static_cast<std::uint8_t>(octet_value);

        const bool is_last_octet = octet_index + 1 == octet_count;
        if (is_last_octet) {
            if (position != text.size()) {
                return std::nullopt;
            }
        } else {
            if (position >= text.size() || text[position] != '.') {
                return std::nullopt;
            }
            ++position;
        }
    }

    return Ipv4Address{octets};
}

std::string Ipv4Address::to_string() const {
    constexpr std::size_t maximum_text_length = 15;
    std::string text;
    text.reserve(maximum_text_length);
    const Octets address_octets = octets();

    for (std::size_t index = 0; index < octet_count; ++index) {
        std::array<char, 3U> octet_text{};
        const auto conversion = std::to_chars(
            octet_text.data(),
            octet_text.data() + octet_text.size(),
            static_cast<unsigned int>(address_octets[index]));
        text.append(octet_text.data(), conversion.ptr);

        if (index + 1 < octet_count) {
            text.push_back('.');
        }
    }

    return text;
}

}  // namespace silicon_switch::network
