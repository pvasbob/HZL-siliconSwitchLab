#pragma once

#include <cstdint>
#include <span>

namespace silicon_switch::network {

[[nodiscard]] std::uint16_t compute_internet_checksum(
    std::span<const std::uint8_t> bytes) noexcept;

[[nodiscard]] bool has_valid_internet_checksum(
    std::span<const std::uint8_t> bytes) noexcept;

}  // namespace silicon_switch::network
