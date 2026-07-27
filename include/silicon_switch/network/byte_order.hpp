#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <type_traits>

namespace silicon_switch::network::wire {

template <typename Integer>
concept UnsignedWireInteger =
    std::unsigned_integral<Integer> &&
    !std::same_as<std::remove_cv_t<Integer>, bool> &&
    (sizeof(Integer) == 1U || sizeof(Integer) == 2U ||
     sizeof(Integer) == 4U || sizeof(Integer) == 8U);

template <UnsignedWireInteger Integer>
[[nodiscard]] constexpr std::optional<Integer> read_big_endian(
    const std::span<const std::uint8_t> bytes,
    const std::size_t offset = 0U) noexcept {
    if (offset > bytes.size() || sizeof(Integer) > bytes.size() - offset) {
        return std::nullopt;
    }

    Integer value = 0U;
    for (std::size_t index = 0; index < sizeof(Integer); ++index) {
        value = static_cast<Integer>(
            (value << 8U) | static_cast<Integer>(bytes[offset + index]));
    }

    return value;
}

template <UnsignedWireInteger Integer>
[[nodiscard]] constexpr bool write_big_endian(
    const Integer value,
    const std::span<std::uint8_t> bytes,
    const std::size_t offset = 0U) noexcept {
    if (offset > bytes.size() || sizeof(Integer) > bytes.size() - offset) {
        return false;
    }

    for (std::size_t index = 0; index < sizeof(Integer); ++index) {
        const std::size_t remaining_bytes = sizeof(Integer) - index - 1U;
        const auto shift = static_cast<unsigned int>(remaining_bytes * 8U);
        bytes[offset + index] =
            static_cast<std::uint8_t>(value >> shift);
    }

    return true;
}

}  // namespace silicon_switch::network::wire
