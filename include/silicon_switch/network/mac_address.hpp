#pragma once

#include <array>
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
        for (const auto byte : bytes_) {
            if (byte != 0xFFU) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] constexpr bool is_multicast() const noexcept {
        return (bytes_.front() & 0x01U) != 0U;
    }

    [[nodiscard]] constexpr bool is_unicast() const noexcept {
        return !is_multicast();
    }

    [[nodiscard]] constexpr bool operator==(const MacAddress& other) const noexcept {
        for (std::size_t index = 0U; index < byte_count; ++index) {
            if (bytes_[index] != other.bytes_[index]) {
                return false;
            }
        }
        return true;
    }
    [[nodiscard]] constexpr bool operator!=(const MacAddress& other) const noexcept {
        return !(*this == other);
    }
    [[nodiscard]] constexpr bool operator<(const MacAddress& other) const noexcept {
        for (std::size_t index = 0U; index < byte_count; ++index) {
            if (bytes_[index] != other.bytes_[index]) {
                return bytes_[index] < other.bytes_[index];
            }
        }
        return false;
    }

private:
    Bytes bytes_;
};

}  // namespace silicon_switch::network
