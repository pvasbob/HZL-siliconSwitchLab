#pragma once

#include "silicon_switch/transport/state_update_protocol.hpp"

#include <chrono>
#include <cstdint>
#include <optional>

namespace silicon_switch::transport {

enum class SynchronizationResult {
    applied_snapshot,
    applied_delta,
    accepted_heartbeat,
    ignored_duplicate,
    ignored_stale,
    needs_snapshot,
    sequence_gap,
    revision_mismatch,
};

class ClientSynchronizer {
public:
    using Clock = std::chrono::steady_clock;

    [[nodiscard]] SynchronizationResult consume(
        const StateUpdate& update, Clock::time_point received_at = Clock::now());
    [[nodiscard]] bool disconnected(
        Clock::time_point now, std::chrono::milliseconds timeout) const noexcept;
    void reset() noexcept;

    [[nodiscard]] bool synchronized() const noexcept { return synchronized_; }
    [[nodiscard]] bool resync_required() const noexcept { return resync_required_; }
    [[nodiscard]] std::optional<std::uint64_t> session_id() const noexcept {
        return session_id_;
    }
    [[nodiscard]] std::uint64_t sequence() const noexcept { return sequence_; }
    [[nodiscard]] std::uint64_t revision() const noexcept { return revision_; }

private:
    std::optional<std::uint64_t> session_id_;
    std::optional<Clock::time_point> last_received_;
    std::uint64_t sequence_{0U};
    std::uint64_t revision_{0U};
    std::uint64_t timestamp_microseconds_{0U};
    bool synchronized_{false};
    bool resync_required_{true};
};

}  // namespace silicon_switch::transport
