#include "silicon_switch/network/ipv4_packet.hpp"

#include "silicon_switch/network/byte_order.hpp"
#include "silicon_switch/network/internet_checksum.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include "silicon_switch/network/byte_span.hpp"
#include <utility>
#include <vector>

namespace silicon_switch::network {
namespace {

constexpr std::uint8_t version_and_header_length = 0x45U;
constexpr std::uint8_t supported_differentiated_services = 0U;
constexpr std::uint16_t dont_fragment_mask = 0x4000U;
constexpr std::uint16_t unsupported_fragmentation_mask = 0xBFFFU;

constexpr std::size_t version_and_header_length_offset = 0U;
constexpr std::size_t differentiated_services_offset = 1U;
constexpr std::size_t total_length_offset = 2U;
constexpr std::size_t identification_offset = 4U;
constexpr std::size_t flags_and_fragment_offset = 6U;
constexpr std::size_t time_to_live_offset = 8U;
constexpr std::size_t protocol_offset = 9U;
constexpr std::size_t checksum_offset = 10U;
constexpr std::size_t source_offset = 12U;
constexpr std::size_t destination_offset = 16U;

[[nodiscard]] Ipv4Address read_address(
    const ByteView bytes,
    const std::size_t offset) {
    const auto value = wire::read_big_endian<std::uint32_t>(bytes, offset);
    return Ipv4Address{*value};
}

[[nodiscard]] bool write_header_field(
    const std::uint16_t value,
    const MutableByteView bytes,
    const std::size_t offset) {
    return wire::write_big_endian<std::uint16_t>(value, bytes, offset);
}

}  // namespace

std::optional<Ipv4Packet> Ipv4Packet::create(
    Ipv4Address source,
    Ipv4Address destination,
    const IpProtocol protocol,
    Payload payload,
    const std::uint8_t time_to_live,
    const std::uint16_t identification,
    const bool dont_fragment) {
    if (payload.size() > maximum_payload_size || time_to_live == 0U) {
        return std::nullopt;
    }

    return Ipv4Packet{
        std::move(source),
        std::move(destination),
        protocol,
        std::move(payload),
        time_to_live,
        identification,
        dont_fragment,
        ValidatedTag{},
    };
}

std::optional<Ipv4Packet> Ipv4Packet::parse(
    const ByteView bytes) {
    if (bytes.size() < header_size ||
        bytes[version_and_header_length_offset] != version_and_header_length ||
        bytes[differentiated_services_offset] !=
            supported_differentiated_services) {
        return std::nullopt;
    }

    const auto total_length =
        wire::read_big_endian<std::uint16_t>(bytes, total_length_offset);
    const auto identification =
        wire::read_big_endian<std::uint16_t>(bytes, identification_offset);
    const auto flags_and_fragment = wire::read_big_endian<std::uint16_t>(
        bytes, flags_and_fragment_offset);
    if (!total_length.has_value() || !identification.has_value() ||
        !flags_and_fragment.has_value() ||
        *total_length != bytes.size() || *total_length < header_size ||
        (*flags_and_fragment & unsupported_fragmentation_mask) != 0U ||
        bytes[time_to_live_offset] == 0U ||
        !has_valid_internet_checksum(bytes.first(header_size))) {
        return std::nullopt;
    }

    const auto payload_begin =
        bytes.begin() + static_cast<std::ptrdiff_t>(header_size);
    Payload payload{payload_begin, bytes.end()};

    return create(
        read_address(bytes, source_offset),
        read_address(bytes, destination_offset),
        static_cast<IpProtocol>(bytes[protocol_offset]),
        std::move(payload),
        bytes[time_to_live_offset],
        *identification,
        (*flags_and_fragment & dont_fragment_mask) != 0U);
}

std::vector<std::uint8_t> Ipv4Packet::serialize() const {
    std::vector<std::uint8_t> bytes(header_size + payload_.size());
    const MutableByteView output{bytes};
    bytes[version_and_header_length_offset] = version_and_header_length;
    bytes[differentiated_services_offset] = supported_differentiated_services;
    bytes[time_to_live_offset] = time_to_live_;
    bytes[protocol_offset] = static_cast<std::uint8_t>(protocol_);

    const auto total_length = static_cast<std::uint16_t>(bytes.size());
    const std::uint16_t flags_and_fragment =
        dont_fragment_ ? dont_fragment_mask : 0U;
    const bool wrote_header =
        write_header_field(total_length, output, total_length_offset) &&
        write_header_field(identification_, output, identification_offset) &&
        write_header_field(
            flags_and_fragment, output, flags_and_fragment_offset) &&
        wire::write_big_endian<std::uint32_t>(
            source_.value(), output, source_offset) &&
        wire::write_big_endian<std::uint32_t>(
            destination_.value(), output, destination_offset);
    if (!wrote_header) {
        return {};
    }

    const std::uint16_t checksum =
        compute_internet_checksum(ByteView{bytes}.first(
            header_size));
    if (!write_header_field(checksum, output, checksum_offset)) {
        return {};
    }

    std::copy(
        payload_.begin(),
        payload_.end(),
        bytes.begin() + static_cast<std::ptrdiff_t>(header_size));
    return bytes;
}

Ipv4Packet::Ipv4Packet(
    Ipv4Address source,
    Ipv4Address destination,
    const IpProtocol protocol,
    Payload payload,
    const std::uint8_t time_to_live,
    const std::uint16_t identification,
    const bool dont_fragment,
    ValidatedTag)
    : source_{std::move(source)},
      destination_{std::move(destination)},
      protocol_{protocol},
      payload_{std::move(payload)},
      time_to_live_{time_to_live},
      identification_{identification},
      dont_fragment_{dont_fragment} {}

}  // namespace silicon_switch::network
