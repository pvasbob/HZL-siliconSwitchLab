#pragma once

#include "silicon_switch/network/byte_span.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace silicon_switch::discovery {

enum class DiscoveryMessageType : std::uint8_t { query = 1U, advertisement = 2U };
enum class MachineRole : std::uint8_t { unspecified = 0U, leader = 1U, observer = 2U };

enum class Capability : std::uint32_t {
    none = 0U,
    cuda = 1U << 0U,
    opengl = 1U << 1U,
    control_client = 1U << 2U,
};

[[nodiscard]] constexpr Capability operator|(const Capability left,
                                             const Capability right) noexcept {
    return static_cast<Capability>(static_cast<std::uint32_t>(left) |
                                   static_cast<std::uint32_t>(right));
}
[[nodiscard]] constexpr bool has_capability(const Capability capabilities,
                                            const Capability capability) noexcept {
    return (static_cast<std::uint32_t>(capabilities) &
            static_cast<std::uint32_t>(capability)) != 0U;
}

enum class DiscoveryParseError {
    truncated,
    invalid_magic,
    unsupported_version,
    unknown_type,
    invalid_role,
    invalid_fields,
    length_mismatch,
};

class DiscoveryMessage {
public:
    static constexpr std::uint16_t current_version = 1U;
    static constexpr std::size_t header_size = 33U;
    static constexpr std::size_t maximum_name_size = 63U;

    [[nodiscard]] static DiscoveryMessage query(std::uint64_t request_id);
    [[nodiscard]] static std::optional<DiscoveryMessage> advertisement(
        std::uint64_t request_id,
        std::uint64_t node_id,
        MachineRole role,
        Capability capabilities,
        std::uint16_t control_port,
        std::uint16_t state_port,
        std::string name);
    [[nodiscard]] static std::variant<DiscoveryMessage, DiscoveryParseError> parse(
        network::ByteView bytes);
    [[nodiscard]] std::vector<std::uint8_t> serialize() const;

    [[nodiscard]] DiscoveryMessageType type() const noexcept { return type_; }
    [[nodiscard]] MachineRole role() const noexcept { return role_; }
    [[nodiscard]] Capability capabilities() const noexcept { return capabilities_; }
    [[nodiscard]] std::uint64_t request_id() const noexcept { return request_id_; }
    [[nodiscard]] std::uint64_t node_id() const noexcept { return node_id_; }
    [[nodiscard]] std::uint16_t control_port() const noexcept { return control_port_; }
    [[nodiscard]] std::uint16_t state_port() const noexcept { return state_port_; }
    [[nodiscard]] const std::string& name() const noexcept { return name_; }

private:
    DiscoveryMessage(DiscoveryMessageType type,
                     std::uint64_t request_id,
                     std::uint64_t node_id,
                     MachineRole role,
                     Capability capabilities,
                     std::uint16_t control_port,
                     std::uint16_t state_port,
                     std::string name);

    DiscoveryMessageType type_;
    MachineRole role_;
    Capability capabilities_;
    std::uint64_t request_id_;
    std::uint64_t node_id_;
    std::uint16_t control_port_;
    std::uint16_t state_port_;
    std::string name_;
};

}  // namespace silicon_switch::discovery
