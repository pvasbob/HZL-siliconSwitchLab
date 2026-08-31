#include "silicon_switch/transport/state_update_protocol.hpp"

#include "silicon_switch/network/byte_order.hpp"

#include <algorithm>
#include <cstddef>
#include <utility>

namespace silicon_switch::transport {
namespace {
constexpr std::uint32_t state_update_magic = 0x53535550U;

bool is_known(const StateUpdateType type) {
    switch (type) {
        case StateUpdateType::snapshot:
        case StateUpdateType::delta:
        case StateUpdateType::heartbeat:
        case StateUpdateType::resync_request:
            return true;
    }
    return false;
}

bool revisions_are_valid(const StateUpdateType type,
                         const std::uint64_t base_revision,
                         const std::uint64_t revision) {
    if (type == StateUpdateType::snapshot) {
        return base_revision == 0U && revision > 0U;
    }
    if (type == StateUpdateType::delta) {
        return revision > base_revision;
    }
    return base_revision <= revision;
}
}  // namespace

std::optional<StateUpdate> StateUpdate::create(
    const StateUpdateType type,
    const std::uint64_t session_id,
    const std::uint64_t sequence,
    const std::uint64_t timestamp_microseconds,
    const std::uint64_t base_revision,
    const std::uint64_t revision,
    std::vector<std::uint8_t> payload) {
    if (!is_known(type) || session_id == 0U || sequence == 0U ||
        payload.size() > maximum_payload_size ||
        !revisions_are_valid(type, base_revision, revision)) {
        return std::nullopt;
    }
    return StateUpdate{type, session_id, sequence, timestamp_microseconds,
                       base_revision, revision, std::move(payload)};
}

std::variant<StateUpdate, StateUpdateParseError> StateUpdate::parse(
    const network::ByteView bytes) {
    if (bytes.size() < header_size) {
        return StateUpdateParseError::truncated;
    }
    const auto magic = network::wire::read_big_endian<std::uint32_t>(bytes, 0U).value();
    const auto version = network::wire::read_big_endian<std::uint16_t>(bytes, 4U).value();
    const auto type = static_cast<StateUpdateType>(
        network::wire::read_big_endian<std::uint16_t>(bytes, 6U).value());
    const auto session = network::wire::read_big_endian<std::uint64_t>(bytes, 8U).value();
    const auto sequence = network::wire::read_big_endian<std::uint64_t>(bytes, 16U).value();
    const auto timestamp = network::wire::read_big_endian<std::uint64_t>(bytes, 24U).value();
    const auto base = network::wire::read_big_endian<std::uint64_t>(bytes, 32U).value();
    const auto revision = network::wire::read_big_endian<std::uint64_t>(bytes, 40U).value();
    const auto length = network::wire::read_big_endian<std::uint32_t>(bytes, 48U).value();
    if (magic != state_update_magic) {
        return StateUpdateParseError::invalid_magic;
    }
    if (version != current_version) {
        return StateUpdateParseError::unsupported_version;
    }
    if (!is_known(type)) {
        return StateUpdateParseError::unknown_type;
    }
    if (length > maximum_payload_size) {
        return StateUpdateParseError::payload_too_large;
    }
    if (bytes.size() != header_size + static_cast<std::size_t>(length)) {
        return StateUpdateParseError::length_mismatch;
    }
    if (session == 0U || sequence == 0U || !revisions_are_valid(type, base, revision)) {
        return StateUpdateParseError::invalid_revision;
    }
    std::vector<std::uint8_t> payload(
        bytes.begin() + static_cast<std::ptrdiff_t>(header_size), bytes.end());
    return StateUpdate{type, session, sequence, timestamp, base, revision,
                       std::move(payload)};
}

std::vector<std::uint8_t> StateUpdate::serialize() const {
    std::vector<std::uint8_t> bytes(header_size + payload_.size());
    network::MutableByteView output{bytes};
    static_cast<void>(network::wire::write_big_endian(state_update_magic, output, 0U));
    static_cast<void>(network::wire::write_big_endian(current_version, output, 4U));
    static_cast<void>(network::wire::write_big_endian(
        static_cast<std::uint16_t>(type_), output, 6U));
    static_cast<void>(network::wire::write_big_endian(session_id_, output, 8U));
    static_cast<void>(network::wire::write_big_endian(sequence_, output, 16U));
    static_cast<void>(network::wire::write_big_endian(timestamp_microseconds_, output, 24U));
    static_cast<void>(network::wire::write_big_endian(base_revision_, output, 32U));
    static_cast<void>(network::wire::write_big_endian(revision_, output, 40U));
    static_cast<void>(network::wire::write_big_endian(
        static_cast<std::uint32_t>(payload_.size()), output, 48U));
    std::copy(payload_.begin(), payload_.end(),
              bytes.begin() + static_cast<std::ptrdiff_t>(header_size));
    return bytes;
}

StateUpdate::StateUpdate(const StateUpdateType type,
                         const std::uint64_t session_id,
                         const std::uint64_t sequence,
                         const std::uint64_t timestamp_microseconds,
                         const std::uint64_t base_revision,
                         const std::uint64_t revision,
                         std::vector<std::uint8_t> payload)
    : type_{type},
      session_id_{session_id},
      sequence_{sequence},
      timestamp_microseconds_{timestamp_microseconds},
      base_revision_{base_revision},
      revision_{revision},
      payload_{std::move(payload)} {}

}  // namespace silicon_switch::transport
