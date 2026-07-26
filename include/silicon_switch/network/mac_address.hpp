#pragma once

#include <array>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace silicon_switch::network {

class MacAddress {
public:
    static constexpr std::size_t byte_count = 6;
    using Bytes = std::array<std::uint8_t, byte_count>;

    explicit constexpr MacAddress(const Bytes bytes) noexcept : bytes_{bytes} {}

    [[nodiscard]] static std::optional<MacAddress>
    parse(std::string_view text) noexcept;

    [[nodiscard]] std::string to_string() const;

    [[nodiscard]] constexpr const Bytes& bytes() const noexcept {
        return bytes_;
    }

    [[nodiscard]] constexpr bool is_broadcast() const noexcept {
        return bytes_ == Bytes{0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU};
    }

    [[nodiscard]] constexpr bool is_multicast() const noexcept {
        return (bytes_.front() & 0x01U) != 0U;
    }

    [[nodiscard]] constexpr bool is_unicast() const noexcept {
        return !is_multicast();
    }

    auto operator<=>(const MacAddress&) const noexcept = default;

private:
    Bytes bytes_;
};

}  // namespace silicon_switch::network
