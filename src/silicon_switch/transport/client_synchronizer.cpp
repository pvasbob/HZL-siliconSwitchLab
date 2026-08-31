#include "silicon_switch/transport/client_synchronizer.hpp"

namespace silicon_switch::transport {

SynchronizationResult ClientSynchronizer::consume(
    const StateUpdate& update, const Clock::time_point received_at) {
    last_received_ = received_at;

    const bool new_session = !session_id_.has_value() ||
                             session_id_.value() != update.session_id();
    if (new_session) {
        session_id_ = update.session_id();
        sequence_ = 0U;
        revision_ = 0U;
        timestamp_microseconds_ = 0U;
        synchronized_ = false;
        resync_required_ = true;
        if (update.type() != StateUpdateType::snapshot) {
            return SynchronizationResult::needs_snapshot;
        }
    } else {
        if (update.sequence() == sequence_) {
            return SynchronizationResult::ignored_duplicate;
        }
        if (update.sequence() < sequence_ ||
            update.timestamp_microseconds() < timestamp_microseconds_) {
            return SynchronizationResult::ignored_stale;
        }
    }

    if (update.type() == StateUpdateType::snapshot) {
        sequence_ = update.sequence();
        revision_ = update.revision();
        timestamp_microseconds_ = update.timestamp_microseconds();
        synchronized_ = true;
        resync_required_ = false;
        return SynchronizationResult::applied_snapshot;
    }

    if (!synchronized_) {
        resync_required_ = true;
        return SynchronizationResult::needs_snapshot;
    }
    if (update.sequence() != sequence_ + 1U) {
        synchronized_ = false;
        resync_required_ = true;
        return SynchronizationResult::sequence_gap;
    }
    if (update.type() == StateUpdateType::delta &&
        update.base_revision() != revision_) {
        synchronized_ = false;
        resync_required_ = true;
        return SynchronizationResult::revision_mismatch;
    }

    sequence_ = update.sequence();
    timestamp_microseconds_ = update.timestamp_microseconds();
    if (update.type() == StateUpdateType::delta) {
        revision_ = update.revision();
        return SynchronizationResult::applied_delta;
    }
    return SynchronizationResult::accepted_heartbeat;
}

bool ClientSynchronizer::disconnected(
    const Clock::time_point now,
    const std::chrono::milliseconds timeout) const noexcept {
    return !last_received_.has_value() || now - last_received_.value() > timeout;
}

void ClientSynchronizer::reset() noexcept {
    session_id_.reset();
    last_received_.reset();
    sequence_ = 0U;
    revision_ = 0U;
    timestamp_microseconds_ = 0U;
    synchronized_ = false;
    resync_required_ = true;
}

}  // namespace silicon_switch::transport
