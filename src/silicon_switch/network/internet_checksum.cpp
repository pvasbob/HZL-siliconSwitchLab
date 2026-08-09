#include "silicon_switch/network/internet_checksum.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace silicon_switch::network {
namespace {

[[nodiscard]] constexpr std::uint32_t fold_carry(
    const std::uint32_t value) noexcept {
    return (value & 0xFFFFU) + (value >> 16U);
}

}  // namespace

std::uint16_t compute_internet_checksum(
    const std::span<const std::uint8_t> bytes) noexcept {
    std::uint32_t sum = 0U;
    std::size_t offset = 0U;

    while (offset + 1U < bytes.size()) {
        const auto word = static_cast<std::uint16_t>(
            (static_cast<std::uint16_t>(bytes[offset]) << 8U) |
            static_cast<std::uint16_t>(bytes[offset + 1U]));
        sum = fold_carry(sum + word);
        offset += 2U;
    }

    if (offset < bytes.size()) {
        const auto final_word = static_cast<std::uint16_t>(
            static_cast<std::uint16_t>(bytes[offset]) << 8U);
        sum = fold_carry(sum + final_word);
    }

    sum = fold_carry(sum);
    return static_cast<std::uint16_t>(~sum & 0xFFFFU);
}

bool has_valid_internet_checksum(
    const std::span<const std::uint8_t> bytes) noexcept {
    return !bytes.empty() && compute_internet_checksum(bytes) == 0U;
}

}  // namespace silicon_switch::network



// The one’s-complement operation is bit inversion using ~. One’s-complement 
// addition is ordinary addition followed by end-around carry. The Internet 
// checksum uses one’s-complement addition for all header words and then applies 
// the ~ one’s-complement operation to the final sum.