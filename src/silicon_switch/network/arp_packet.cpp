#include "silicon_switch/network/arp_packet.hpp"

#include "silicon_switch/network/byte_order.hpp"
#include "silicon_switch/network/ether_type.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace silicon_switch::network {
namespace {

constexpr std::uint16_t ethernet_hardware_type = 1U;
constexpr std::uint8_t ethernet_address_length = 6U;
constexpr std::uint8_t ipv4_address_length = 4U;

constexpr std::size_t hardware_type_offset = 0U;
constexpr std::size_t protocol_type_offset = 2U;
constexpr std::size_t hardware_length_offset = 4U;
constexpr std::size_t protocol_length_offset = 5U;
constexpr std::size_t operation_offset = 6U;
constexpr std::size_t sender_mac_offset = 8U;
constexpr std::size_t sender_ip_offset = 14U;
constexpr std::size_t target_mac_offset = 18U;
constexpr std::size_t target_ip_offset = 24U;

constexpr MacAddress::Bytes unspecified_mac_bytes{};

[[nodiscard]] constexpr MacAddress unspecified_mac() noexcept {
    return MacAddress{unspecified_mac_bytes};
}

[[nodiscard]] bool is_valid_unicast(const MacAddress& address) noexcept {
    return address != unspecified_mac() && address.is_unicast();
}

[[nodiscard]] MacAddress read_mac(
    const ByteView bytes,
    const std::size_t offset) {
    MacAddress::Bytes address{};
    std::copy_n(
        bytes.begin() + static_cast<std::ptrdiff_t>(offset),
        MacAddress::byte_count,
        address.begin());
    return MacAddress{address};
}

void write_mac(
    const MacAddress& address,
    const MutableByteView bytes,
    const std::size_t offset) {
    std::copy(address.bytes().begin(),
              address.bytes().end(),
              bytes.begin() + static_cast<std::ptrdiff_t>(offset));
}

[[nodiscard]] std::optional<ArpPacket> create_parsed_packet(
    const ArpOperation operation,
    const MacAddress sender_mac,
    const Ipv4Address sender_ip,
    const MacAddress target_mac,
    const Ipv4Address target_ip) {
    if (operation == ArpOperation::request) {
        if (target_mac != unspecified_mac()) {
            return std::nullopt;
        }
        return ArpPacket::create_request(sender_mac, sender_ip, target_ip);
    }

    return ArpPacket::create_reply(
        sender_mac, sender_ip, target_mac, target_ip);
}

}  // namespace

std::optional<ArpPacket> ArpPacket::create_request(
    MacAddress sender_mac,
    const Ipv4Address sender_ip,
    const Ipv4Address target_ip) {
    if (!is_valid_unicast(sender_mac)) {
        return std::nullopt;
    }

    return ArpPacket{
        ArpOperation::request,
        std::move(sender_mac),
        sender_ip,
        unspecified_mac(),
        target_ip,
        ValidatedTag{}};
}

std::optional<ArpPacket> ArpPacket::create_reply(
    MacAddress sender_mac,
    const Ipv4Address sender_ip,
    MacAddress target_mac,
    const Ipv4Address target_ip) {
    if (!is_valid_unicast(sender_mac) || !is_valid_unicast(target_mac)) {
        return std::nullopt;
    }

    return ArpPacket{
        ArpOperation::reply,
        std::move(sender_mac),
        sender_ip,
        std::move(target_mac),
        target_ip,
        ValidatedTag{}};
}

std::optional<ArpPacket> ArpPacket::parse(const ByteView bytes) {
    if (bytes.size() != serialized_size) {
        return std::nullopt;
    }

    const auto hardware_type =
        wire::read_big_endian<std::uint16_t>(bytes, hardware_type_offset);
    const auto protocol_type =
        wire::read_big_endian<std::uint16_t>(bytes, protocol_type_offset);
    const auto operation =
        wire::read_big_endian<std::uint16_t>(bytes, operation_offset);
    const auto sender_ip =
        wire::read_big_endian<std::uint32_t>(bytes, sender_ip_offset);
    const auto target_ip =
        wire::read_big_endian<std::uint32_t>(bytes, target_ip_offset);

    if (!hardware_type.has_value() || !protocol_type.has_value() ||
        !operation.has_value() || !sender_ip.has_value() ||
        !target_ip.has_value() || *hardware_type != ethernet_hardware_type ||
        *protocol_type != static_cast<std::uint16_t>(EtherType::ipv4) ||
        bytes[hardware_length_offset] != ethernet_address_length ||
        bytes[protocol_length_offset] != ipv4_address_length ||
        (*operation != static_cast<std::uint16_t>(ArpOperation::request) &&
         *operation != static_cast<std::uint16_t>(ArpOperation::reply))) {
        return std::nullopt;
    }

    return create_parsed_packet(
        static_cast<ArpOperation>(*operation),
        read_mac(bytes, sender_mac_offset),
        Ipv4Address{*sender_ip},
        read_mac(bytes, target_mac_offset),
        Ipv4Address{*target_ip});
}

std::vector<std::uint8_t> ArpPacket::serialize() const {
    std::vector<std::uint8_t> bytes(serialized_size);
    const MutableByteView output{bytes};

    const bool wrote_fields =
        wire::write_big_endian<std::uint16_t>(
            ethernet_hardware_type, output, hardware_type_offset) &&
        wire::write_big_endian<std::uint16_t>(
            static_cast<std::uint16_t>(EtherType::ipv4),
            output,
            protocol_type_offset) &&
        wire::write_big_endian<std::uint16_t>(
            static_cast<std::uint16_t>(operation_), output, operation_offset) &&
        wire::write_big_endian<std::uint32_t>(
            sender_ip_.value(), output, sender_ip_offset) &&
        wire::write_big_endian<std::uint32_t>(
            target_ip_.value(), output, target_ip_offset);
    if (!wrote_fields) {
        return {};
    }

    bytes[hardware_length_offset] = ethernet_address_length;
    bytes[protocol_length_offset] = ipv4_address_length;
    write_mac(sender_mac_, output, sender_mac_offset);
    write_mac(target_mac_, output, target_mac_offset);
    return bytes;
}

ArpPacket::ArpPacket(
    const ArpOperation operation,
    MacAddress sender_mac,
    const Ipv4Address sender_ip,
    MacAddress target_mac,
    const Ipv4Address target_ip,
    ValidatedTag) noexcept
    : operation_{operation},
      sender_mac_{std::move(sender_mac)},
      sender_ip_{sender_ip},
      target_mac_{std::move(target_mac)},
      target_ip_{target_ip} {}

}  // namespace silicon_switch::network
