#pragma once

#include "silicon_switch/network/ipv4_address.hpp"

#include <compare>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace silicon_switch::network {

class Ipv4Prefix {
public:
    using Length = std::uint8_t;
    static constexpr Length maximum_length = 32U;

    [[nodiscard]] static constexpr std::optional<Ipv4Prefix> create(
        const Ipv4Address address,
        const Length length) noexcept {
        if (length > maximum_length) {
            return std::nullopt;
        }

        return Ipv4Prefix{address, length, ValidatedTag{}};
    }

    [[nodiscard]] static std::optional<Ipv4Prefix>
    parse(std::string_view text) noexcept;

    [[nodiscard]] std::string to_string() const;

    [[nodiscard]] constexpr Ipv4Address network_address() const noexcept {
        return network_address_;
    }

    [[nodiscard]] constexpr Length length() const noexcept {
        return length_;
    }

    [[nodiscard]] constexpr Ipv4Address subnet_mask() const noexcept {
        return Ipv4Address{mask_value(length_)};
    }

    [[nodiscard]] constexpr Ipv4Address last_address() const noexcept {
        return Ipv4Address{network_address_.value() | ~mask_value(length_)};
    }

    [[nodiscard]] constexpr bool contains(
        const Ipv4Address address) const noexcept {
        return (address.value() & mask_value(length_)) ==
               network_address_.value();
    }

    auto operator<=>(const Ipv4Prefix&) const noexcept = default;

private:
    struct ValidatedTag {};

    explicit constexpr Ipv4Prefix(
        const Ipv4Address address,
        const Length length,
        ValidatedTag) noexcept
        : network_address_{address.value() & mask_value(length)},
          length_{length} {}

    [[nodiscard]] static constexpr Ipv4Address::Value
    mask_value(const Length length) noexcept {
        if (length == 0U) {
            return 0U;
        }

        return 0xFFFFFFFFU << (maximum_length - length);
    }

    Ipv4Address network_address_;
    Length length_;
};

}  // namespace silicon_switch::network
