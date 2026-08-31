#include "silicon_switch/visualization/observer_client.hpp"

#include "silicon_switch/transport/state_update_protocol.hpp"

#include <utility>

namespace silicon_switch::visualization {

ObserverClient::BindResult ObserverClient::bind(const std::string& address,
                                                const std::uint16_t port) {
    auto result = transport::UdpSocket::bind_ipv4(address, port);
    if (std::holds_alternative<transport::UdpError>(result)) {
        return std::get<transport::UdpError>(result);
    }
    return ObserverClient{std::get<transport::UdpSocket>(std::move(result))};
}

ObserverResult ObserverClient::receive_next() {
    auto received = socket_.receive(transport::StateUpdate::header_size +
                                    transport::StateUpdate::maximum_payload_size);
    if (std::holds_alternative<transport::UdpError>(received)) {
        return ObserverResult::network_error;
    }
    return consume_datagram(std::get<transport::UdpDatagram>(received).payload);
}

ObserverResult ObserverClient::consume_datagram(const network::ByteView bytes) {
    const auto parsed = transport::StateUpdate::parse(bytes);
    if (!std::holds_alternative<transport::StateUpdate>(parsed)) {
        return ObserverResult::malformed_update;
    }
    const auto& update = std::get<transport::StateUpdate>(parsed);

    TopologyScene candidate = scene_;
    if (update.type() == transport::StateUpdateType::snapshot) {
        auto snapshot = TopologyScene::parse_snapshot(update.payload());
        if (!snapshot.has_value()) {
            return ObserverResult::malformed_scene;
        }
        candidate = std::move(snapshot.value());
    } else if (update.type() == transport::StateUpdateType::delta) {
        if (!candidate.apply_delta(update.payload())) {
            return ObserverResult::malformed_scene;
        }
    }

    const auto result = synchronizer_.consume(update);
    switch (result) {
        case transport::SynchronizationResult::applied_snapshot:
            scene_ = std::move(candidate);
            return ObserverResult::applied_snapshot;
        case transport::SynchronizationResult::applied_delta:
            scene_ = std::move(candidate);
            return ObserverResult::applied_delta;
        case transport::SynchronizationResult::accepted_heartbeat:
        case transport::SynchronizationResult::ignored_duplicate:
        case transport::SynchronizationResult::ignored_stale:
            return ObserverResult::ignored;
        case transport::SynchronizationResult::needs_snapshot:
        case transport::SynchronizationResult::sequence_gap:
        case transport::SynchronizationResult::revision_mismatch:
            return ObserverResult::resync_required;
    }
    return ObserverResult::resync_required;
}

}  // namespace silicon_switch::visualization
