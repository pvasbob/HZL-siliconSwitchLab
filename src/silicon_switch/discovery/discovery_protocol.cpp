#include "silicon_switch/discovery/discovery_protocol.hpp"

#include "silicon_switch/network/byte_order.hpp"

#include <algorithm>
#include <cstddef>
#include <utility>

namespace silicon_switch::discovery {
namespace {
constexpr std::uint32_t discovery_magic = 0x53534431U;
constexpr std::uint32_t known_capabilities = 0x7U;

bool valid_role(const MachineRole role) {
    return role == MachineRole::unspecified || role == MachineRole::leader ||
           role == MachineRole::observer;
}
}  // namespace

DiscoveryMessage DiscoveryMessage::query(const std::uint64_t request_id) {
    return {DiscoveryMessageType::query, request_id, 0U, MachineRole::unspecified,
            Capability::none, 0U, 0U, {}};
}

std::optional<DiscoveryMessage> DiscoveryMessage::advertisement(
    const std::uint64_t request_id,
    const std::uint64_t node_id,
    const MachineRole role,
    const Capability capabilities,
    const std::uint16_t control_port,
    const std::uint16_t state_port,
    std::string name) {
    const bool valid_ports = state_port != 0U &&
                             (role != MachineRole::leader || control_port != 0U);
    if (request_id == 0U || node_id == 0U || role == MachineRole::unspecified ||
        !valid_role(role) || !valid_ports || name.empty() ||
        name.size() > maximum_name_size ||
        (static_cast<std::uint32_t>(capabilities) & ~known_capabilities) != 0U) {
        return std::nullopt;
    }
    return DiscoveryMessage{DiscoveryMessageType::advertisement, request_id, node_id,
                            role, capabilities, control_port, state_port,
                            std::move(name)};
}

std::variant<DiscoveryMessage, DiscoveryParseError> DiscoveryMessage::parse(
    const network::ByteView bytes) {
    if (bytes.size() < header_size) {
        return DiscoveryParseError::truncated;
    }
    const auto magic = network::wire::read_big_endian<std::uint32_t>(bytes, 0U).value();
    const auto version = network::wire::read_big_endian<std::uint16_t>(bytes, 4U).value();
    const auto type = static_cast<DiscoveryMessageType>(bytes[6U]);
    const auto role = static_cast<MachineRole>(bytes[7U]);
    const auto capabilities = static_cast<Capability>(
        network::wire::read_big_endian<std::uint32_t>(bytes, 8U).value());
    const auto request_id = network::wire::read_big_endian<std::uint64_t>(bytes, 12U).value();
    const auto node_id = network::wire::read_big_endian<std::uint64_t>(bytes, 20U).value();
    const auto control_port = network::wire::read_big_endian<std::uint16_t>(bytes, 28U).value();
    const auto state_port = network::wire::read_big_endian<std::uint16_t>(bytes, 30U).value();
    const auto name_size = static_cast<std::size_t>(bytes[32U]);
    if (magic != discovery_magic) {
        return DiscoveryParseError::invalid_magic;
    }
    if (version != current_version) {
        return DiscoveryParseError::unsupported_version;
    }
    if (type != DiscoveryMessageType::query && type != DiscoveryMessageType::advertisement) {
        return DiscoveryParseError::unknown_type;
    }
    if (!valid_role(role)) {
        return DiscoveryParseError::invalid_role;
    }
    if (name_size > maximum_name_size || bytes.size() != header_size + name_size) {
        return DiscoveryParseError::length_mismatch;
    }
    std::string name(bytes.begin() + static_cast<std::ptrdiff_t>(header_size), bytes.end());
    if (type == DiscoveryMessageType::query) {
        if (request_id == 0U || node_id != 0U || role != MachineRole::unspecified ||
            control_port != 0U || state_port != 0U || !name.empty()) {
            return DiscoveryParseError::invalid_fields;
        }
        return query(request_id);
    }
    auto message = advertisement(request_id, node_id, role, capabilities,
                                 control_port, state_port, std::move(name));
    if (!message.has_value()) {
        return DiscoveryParseError::invalid_fields;
    }
    return std::move(message.value());
}

std::vector<std::uint8_t> DiscoveryMessage::serialize() const {
    std::vector<std::uint8_t> bytes(header_size + name_.size());
    network::MutableByteView output{bytes};
    static_cast<void>(network::wire::write_big_endian(discovery_magic, output, 0U));
    static_cast<void>(network::wire::write_big_endian(current_version, output, 4U));
    bytes[6U] = static_cast<std::uint8_t>(type_);
    bytes[7U] = static_cast<std::uint8_t>(role_);
    static_cast<void>(network::wire::write_big_endian(
        static_cast<std::uint32_t>(capabilities_), output, 8U));
    static_cast<void>(network::wire::write_big_endian(request_id_, output, 12U));
    static_cast<void>(network::wire::write_big_endian(node_id_, output, 20U));
    static_cast<void>(network::wire::write_big_endian(control_port_, output, 28U));
    static_cast<void>(network::wire::write_big_endian(state_port_, output, 30U));
    bytes[32U] = static_cast<std::uint8_t>(name_.size());
    std::copy(name_.begin(), name_.end(),
              bytes.begin() + static_cast<std::ptrdiff_t>(header_size));
    return bytes;
}

DiscoveryMessage::DiscoveryMessage(const DiscoveryMessageType type,
                                   const std::uint64_t request_id,
                                   const std::uint64_t node_id,
                                   const MachineRole role,
                                   const Capability capabilities,
                                   const std::uint16_t control_port,
                                   const std::uint16_t state_port,
                                   std::string name)
    : type_{type},
      role_{role},
      capabilities_{capabilities},
      request_id_{request_id},
      node_id_{node_id},
      control_port_{control_port},
      state_port_{state_port},
      name_{std::move(name)} {}

}  // namespace silicon_switch::discovery
