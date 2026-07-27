#include "silicon_switch/network/ethernet_frame.hpp"

#include "silicon_switch/network/byte_order.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace silicon_switch::network {
namespace {

constexpr std::size_t destination_offset = 0U;
constexpr std::size_t source_offset = destination_offset + MacAddress::byte_count;
constexpr std::size_t ether_type_offset = source_offset + MacAddress::byte_count;

[[nodiscard]] MacAddress read_mac_address(
    const std::span<const std::uint8_t> bytes,
    const std::size_t offset) {
    MacAddress::Bytes address_bytes{};
    std::copy_n(
        bytes.begin() + static_cast<std::ptrdiff_t>(offset),
        MacAddress::byte_count,
        address_bytes.begin());
    return MacAddress{address_bytes};
}

void write_mac_address(
    const MacAddress& address,
    const std::span<std::uint8_t> bytes,
    const std::size_t offset) {
    std::copy(
        address.bytes().begin(),
        address.bytes().end(),
        bytes.begin() + static_cast<std::ptrdiff_t>(offset));
}

}  // namespace

std::optional<EthernetFrame> EthernetFrame::create(
    MacAddress destination,
    MacAddress source,
    const EtherType ether_type,
    Payload payload) {
    if (payload.size() > maximum_payload_size) {
        return std::nullopt;
    }

    return EthernetFrame{
        std::move(destination),
        std::move(source),
        ether_type,
        std::move(payload),
        ValidatedTag{},
    };
}

std::optional<EthernetFrame> EthernetFrame::parse(
    const std::span<const std::uint8_t> bytes) {
    if (bytes.size() < header_size ||
        bytes.size() - header_size > maximum_payload_size) {
        return std::nullopt;
    }

    const auto ether_type_value =
        wire::read_big_endian<std::uint16_t>(bytes, ether_type_offset);
    if (!ether_type_value.has_value()) {
        return std::nullopt;
    }

    const auto payload_begin =
        bytes.begin() + static_cast<std::ptrdiff_t>(header_size);
    Payload payload{payload_begin, bytes.end()};

    return create(
        read_mac_address(bytes, destination_offset),
        read_mac_address(bytes, source_offset),
        static_cast<EtherType>(*ether_type_value),
        std::move(payload));
}

std::vector<std::uint8_t> EthernetFrame::serialize() const {
    std::vector<std::uint8_t> bytes(header_size + payload_.size());
    const std::span<std::uint8_t> output{bytes};

    write_mac_address(destination_, output, destination_offset);
    write_mac_address(source_, output, source_offset);
    const bool wrote_ether_type = wire::write_big_endian<std::uint16_t>(
        static_cast<std::uint16_t>(ether_type_),
        output,
        ether_type_offset);

    if (!wrote_ether_type) {
        return {};
    }

    std::copy(
        payload_.begin(),
        payload_.end(),
        bytes.begin() + static_cast<std::ptrdiff_t>(header_size));
    return bytes;
}

EthernetFrame::EthernetFrame(
    MacAddress destination,
    MacAddress source,
    const EtherType ether_type,
    Payload payload,
    ValidatedTag)
    : destination_{std::move(destination)},
      source_{std::move(source)},
      ether_type_{ether_type},
      payload_{std::move(payload)} {}

}  // namespace silicon_switch::network
