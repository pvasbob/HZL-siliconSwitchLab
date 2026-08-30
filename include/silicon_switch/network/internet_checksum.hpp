#pragma once

#include <cstdint>
#include "silicon_switch/network/byte_span.hpp"

namespace silicon_switch::network {

[[nodiscard]] std::uint16_t compute_internet_checksum(
    ByteView bytes) noexcept;

[[nodiscard]] bool has_valid_internet_checksum(
    ByteView bytes) noexcept;

}  // namespace silicon_switch::network
