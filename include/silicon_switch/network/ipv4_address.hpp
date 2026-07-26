#pragma once

#include <array>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace silicon_switch::network {

class Ipv4Address {
public:
    static constexpr std::size_t octet_count = 4;
    using Octets = std::array<std::uint8_t, octet_count>;
    using Value = std::uint32_t;

    explicit constexpr Ipv4Address(const Octets octets) noexcept
        : value_{(static_cast<Value>(octets[0]) << 24U) |
                 (static_cast<Value>(octets[1]) << 16U) |
                 (static_cast<Value>(octets[2]) << 8U) |
                 static_cast<Value>(octets[3])} {}

    explicit constexpr Ipv4Address(const Value value) noexcept : value_{value} {}

    [[nodiscard]] static std::optional<Ipv4Address>
    parse(std::string_view text) noexcept;

    [[nodiscard]] std::string to_string() const;

    [[nodiscard]] constexpr Value value() const noexcept {
        return value_;
    }

    [[nodiscard]] constexpr Octets octets() const noexcept {
        return Octets{
            static_cast<std::uint8_t>(value_ >> 24U),
            static_cast<std::uint8_t>((value_ >> 16U) & 0xFFU),
            static_cast<std::uint8_t>((value_ >> 8U) & 0xFFU),
            static_cast<std::uint8_t>(value_ & 0xFFU),
        };
    }

    [[nodiscard]] constexpr bool is_unspecified() const noexcept {
        return value_ == 0U;
    }

    [[nodiscard]] constexpr bool is_loopback() const noexcept {
        return (value_ & 0xFF000000U) == 0x7F000000U;
    }

    [[nodiscard]] constexpr bool is_multicast() const noexcept {
        return (value_ & 0xF0000000U) == 0xE0000000U;
    }

    [[nodiscard]] constexpr bool is_limited_broadcast() const noexcept {
        return value_ == 0xFFFFFFFFU;
    }

    auto operator<=>(const Ipv4Address&) const noexcept = default;

private:
    Value value_;
};

}  // namespace silicon_switch::network
