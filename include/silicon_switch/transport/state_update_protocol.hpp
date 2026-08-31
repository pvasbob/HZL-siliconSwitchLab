#pragma once

#include "silicon_switch/network/byte_span.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <variant>
#include <vector>

namespace silicon_switch::transport {

enum class StateUpdateType : std::uint16_t {
    snapshot = 1U,
    delta = 2U,
    heartbeat = 3U,
    resync_request = 4U,
};

enum class StateUpdateParseError {
    truncated,
    invalid_magic,
    unsupported_version,
    unknown_type,
    payload_too_large,
    length_mismatch,
    invalid_revision,
};

class StateUpdate {
public:
    static constexpr std::uint16_t current_version = 1U;
    static constexpr std::size_t header_size = 52U;
    static constexpr std::size_t maximum_payload_size = 60U * 1024U;

    [[nodiscard]] static std::optional<StateUpdate> create(
        StateUpdateType type,
        std::uint64_t session_id,
        std::uint64_t sequence,
        std::uint64_t timestamp_microseconds,
        std::uint64_t base_revision,
        std::uint64_t revision,
        std::vector<std::uint8_t> payload = {});
    [[nodiscard]] static std::variant<StateUpdate, StateUpdateParseError> parse(
        network::ByteView bytes);
    [[nodiscard]] std::vector<std::uint8_t> serialize() const;

    [[nodiscard]] constexpr StateUpdateType type() const noexcept { return type_; }
    [[nodiscard]] constexpr std::uint64_t session_id() const noexcept { return session_id_; }
    [[nodiscard]] constexpr std::uint64_t sequence() const noexcept { return sequence_; }
    [[nodiscard]] constexpr std::uint64_t timestamp_microseconds() const noexcept {
        return timestamp_microseconds_;
    }
    [[nodiscard]] constexpr std::uint64_t base_revision() const noexcept {
        return base_revision_;
    }
    [[nodiscard]] constexpr std::uint64_t revision() const noexcept { return revision_; }
    [[nodiscard]] const std::vector<std::uint8_t>& payload() const noexcept { return payload_; }

private:
    StateUpdate(StateUpdateType type,
                std::uint64_t session_id,
                std::uint64_t sequence,
                std::uint64_t timestamp_microseconds,
                std::uint64_t base_revision,
                std::uint64_t revision,
                std::vector<std::uint8_t> payload);

    StateUpdateType type_;
    std::uint64_t session_id_;
    std::uint64_t sequence_;
    std::uint64_t timestamp_microseconds_;
    std::uint64_t base_revision_;
    std::uint64_t revision_;
    std::vector<std::uint8_t> payload_;
};

}  // namespace silicon_switch::transport
