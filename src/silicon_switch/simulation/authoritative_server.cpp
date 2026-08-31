#include "silicon_switch/simulation/authoritative_server.hpp"

#include <stdexcept>
#include <utility>

namespace silicon_switch::simulation {

AuthoritativeServer::AuthoritativeServer(std::unique_ptr<SimulationBackend> backend,
                                         transport::UdpSocket socket,
                                         const std::uint64_t session_id,
                                         const std::size_t snapshot_interval)
    : backend_{std::move(backend)},
      socket_{std::move(socket)},
      session_id_{session_id},
      snapshot_interval_{snapshot_interval == 0U ? 1U : snapshot_interval} {
    if (backend_ == nullptr) {
        throw std::invalid_argument{"authoritative server requires a simulation backend"};
    }
    if (session_id_ == 0U) {
        throw std::invalid_argument{"authoritative server session ID cannot be zero"};
    }
}

void AuthoritativeServer::add_observer(ObserverEndpoint endpoint) {
    observers_.push_back(std::move(endpoint));
}

std::variant<transport::StateUpdate, PublishError> AuthoritativeServer::next_update(
    const std::uint64_t timestamp_microseconds) {
    const auto previous_revision = revision_;
    ++sequence_;
    ++revision_;
    auto payload = backend_->step(revision_).serialize_snapshot();
    if (payload.empty()) {
        return PublishError::serialization_failed;
    }
    const bool snapshot = sequence_ == 1U || sequence_ % snapshot_interval_ == 0U;
    auto update = transport::StateUpdate::create(
        snapshot ? transport::StateUpdateType::snapshot : transport::StateUpdateType::delta,
        session_id_, sequence_, timestamp_microseconds,
        snapshot ? 0U : previous_revision, revision_, std::move(payload));
    if (!update.has_value()) {
        return PublishError::serialization_failed;
    }
    return std::move(update.value());
}

std::variant<std::size_t, PublishError> AuthoritativeServer::publish_next(
    const std::uint64_t timestamp_microseconds) {
    auto result = next_update(timestamp_microseconds);
    if (std::holds_alternative<PublishError>(result)) {
        return std::get<PublishError>(result);
    }
    const auto bytes = std::get<transport::StateUpdate>(result).serialize();
    std::size_t published = 0U;
    for (const auto& observer : observers_) {
        const auto sent = socket_.send_to_ipv4(observer.address, observer.port, bytes);
        if (!std::holds_alternative<std::size_t>(sent)) {
            return PublishError::socket_error;
        }
        ++published;
    }
    return published;
}

}  // namespace silicon_switch::simulation
