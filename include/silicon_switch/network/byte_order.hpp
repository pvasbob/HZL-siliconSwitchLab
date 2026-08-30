#pragma once

#include "silicon_switch/network/byte_span.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <type_traits>

namespace silicon_switch::network::wire {

template <typename Integer>
[[nodiscard]] constexpr std::optional<Integer> read_big_endian(
    const ByteView bytes,
    const std::size_t offset = 0U) noexcept {
    static_assert(std::is_unsigned<Integer>::value, "wire integer must be unsigned");
    static_assert(!std::is_same<typename std::remove_cv<Integer>::type, bool>::value,
                  "bool is not a wire integer");
    static_assert(sizeof(Integer) == 1U || sizeof(Integer) == 2U ||
                      sizeof(Integer) == 4U || sizeof(Integer) == 8U,
                  "unsupported wire integer width");
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

template <typename Integer>
[[nodiscard]] constexpr bool write_big_endian(
    const Integer value,
    const MutableByteView bytes,
    const std::size_t offset = 0U) noexcept {
    static_assert(std::is_unsigned<Integer>::value, "wire integer must be unsigned");
    static_assert(!std::is_same<typename std::remove_cv<Integer>::type, bool>::value,
                  "bool is not a wire integer");
    static_assert(sizeof(Integer) == 1U || sizeof(Integer) == 2U ||
                      sizeof(Integer) == 4U || sizeof(Integer) == 8U,
                  "unsupported wire integer width");
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
