#include "silicon_switch/discovery/discovery_service.hpp"

#include <algorithm>
#include <utility>

namespace silicon_switch::discovery {

std::variant<std::string, DiscoveryServiceError> DiscoveryResponder::respond_once() {
    auto received = socket_.receive(DiscoveryMessage::header_size +
                                    DiscoveryMessage::maximum_name_size);
    if (!std::holds_alternative<transport::UdpDatagram>(received)) {
        return DiscoveryServiceError::network_error;
    }
    const auto& datagram = std::get<transport::UdpDatagram>(received);
    const auto parsed = DiscoveryMessage::parse(datagram.payload);
    if (!std::holds_alternative<DiscoveryMessage>(parsed)) {
        return DiscoveryServiceError::malformed_message;
    }
    const auto& query = std::get<DiscoveryMessage>(parsed);
    if (query.type() != DiscoveryMessageType::query) {
        return DiscoveryServiceError::unexpected_message;
    }
    const auto response = DiscoveryMessage::advertisement(
        query.request_id(), advertisement_.node_id(), advertisement_.role(),
        advertisement_.capabilities(), advertisement_.control_port(),
        advertisement_.state_port(), advertisement_.name());
    if (!response.has_value()) {
        return DiscoveryServiceError::malformed_message;
    }
    const auto sent = socket_.send_to_ipv4(datagram.source_address, datagram.source_port,
                                           response->serialize());
    if (!std::holds_alternative<std::size_t>(sent)) {
        return DiscoveryServiceError::network_error;
    }
    return datagram.source_address;
}

std::variant<DiscoveredMachine, DiscoveryServiceError> DiscoveryClient::discover_one(
    const std::string& destination,
    const std::uint16_t port,
    const std::uint64_t request_id) {
    const auto query = DiscoveryMessage::query(request_id).serialize();
    const auto sent = socket_.send_to_ipv4(destination, port, query);
    if (!std::holds_alternative<std::size_t>(sent)) {
        return DiscoveryServiceError::network_error;
    }
    auto received = socket_.receive(DiscoveryMessage::header_size +
                                    DiscoveryMessage::maximum_name_size);
    if (!std::holds_alternative<transport::UdpDatagram>(received)) {
        return DiscoveryServiceError::network_error;
    }
    auto datagram = std::get<transport::UdpDatagram>(std::move(received));
    const auto parsed = DiscoveryMessage::parse(datagram.payload);
    if (!std::holds_alternative<DiscoveryMessage>(parsed)) {
        return DiscoveryServiceError::malformed_message;
    }
    const auto& advertisement = std::get<DiscoveryMessage>(parsed);
    if (advertisement.type() != DiscoveryMessageType::advertisement) {
        return DiscoveryServiceError::unexpected_message;
    }
    if (advertisement.request_id() != request_id) {
        return DiscoveryServiceError::request_mismatch;
    }
    return DiscoveredMachine{advertisement.node_id(), advertisement.name(),
                             std::move(datagram.source_address), advertisement.role(),
                             advertisement.capabilities(), advertisement.control_port(),
                             advertisement.state_port()};
}

std::variant<std::vector<DiscoveredMachine>, DiscoveryServiceError>
DiscoveryClient::discover_all(const std::string& destination,
                              const std::uint16_t port,
                              const std::uint64_t request_id,
                              const std::size_t maximum_results) {
    const auto query = DiscoveryMessage::query(request_id).serialize();
    if (!std::holds_alternative<std::size_t>(
            socket_.send_to_ipv4(destination, port, query))) {
        return DiscoveryServiceError::network_error;
    }
    std::vector<DiscoveredMachine> machines;
    while (machines.size() < maximum_results) {
        auto received = socket_.receive(DiscoveryMessage::header_size +
                                        DiscoveryMessage::maximum_name_size);
        if (!std::holds_alternative<transport::UdpDatagram>(received)) {
            break;
        }
        auto datagram = std::get<transport::UdpDatagram>(std::move(received));
        const auto parsed = DiscoveryMessage::parse(datagram.payload);
        if (!std::holds_alternative<DiscoveryMessage>(parsed)) {
            continue;
        }
        const auto& advertisement = std::get<DiscoveryMessage>(parsed);
        if (advertisement.type() != DiscoveryMessageType::advertisement ||
            advertisement.request_id() != request_id) {
            continue;
        }
        const auto duplicate = std::find_if(machines.begin(), machines.end(),
            [&advertisement](const auto& machine) {
                return machine.node_id == advertisement.node_id();
            });
        if (duplicate == machines.end()) {
            machines.push_back({advertisement.node_id(), advertisement.name(),
                                std::move(datagram.source_address), advertisement.role(),
                                advertisement.capabilities(), advertisement.control_port(),
                                advertisement.state_port()});
        }
    }
    return machines;
}

}  // namespace silicon_switch::discovery
