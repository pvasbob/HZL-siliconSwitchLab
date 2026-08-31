#include "discovery/discovery_test.hpp"

#include "silicon_switch/discovery/discovery_service.hpp"
#include "silicon_switch/transport/socket_handle.hpp"
#include "test_support.hpp"

#include <cstdint>
#include <string>
#include <thread>
#include <variant>

namespace silicon_switch::test {

void run_discovery_tests(TestSuite& suite) {
    const auto capabilities = discovery::Capability::cuda |
                              discovery::Capability::opengl;
    const auto advertisement = discovery::DiscoveryMessage::advertisement(
        55U, 1001U, discovery::MachineRole::leader, capabilities,
        7000U, 8000U, "gpu-desktop");
    suite.expect_true(advertisement.has_value(), "create machine advertisement");
    const auto bytes = advertisement->serialize();
    const auto parsed = discovery::DiscoveryMessage::parse(bytes);
    suite.expect_true(std::holds_alternative<discovery::DiscoveryMessage>(parsed),
                      "parse machine advertisement");
    if (std::holds_alternative<discovery::DiscoveryMessage>(parsed)) {
        const auto& message = std::get<discovery::DiscoveryMessage>(parsed);
        suite.expect_equal(message.node_id(), std::uint64_t{1001U},
                           "preserve discovered node identity");
        suite.expect_equal(message.role(), discovery::MachineRole::leader,
                           "preserve discovered machine role");
        suite.expect_true(discovery::has_capability(message.capabilities(),
                                                    discovery::Capability::cuda),
                          "advertise CUDA capability");
        suite.expect_equal(message.name(), std::string{"gpu-desktop"},
                           "preserve discovered machine name");
    }
    const auto query = discovery::DiscoveryMessage::query(99U);
    suite.expect_equal(std::get<discovery::DiscoveryMessage>(
                           discovery::DiscoveryMessage::parse(query.serialize())).request_id(),
                       std::uint64_t{99U}, "round-trip discovery query");
    auto bad_magic = bytes;
    bad_magic[0U] = 0U;
    suite.expect_equal(std::get<discovery::DiscoveryParseError>(
                           discovery::DiscoveryMessage::parse(bad_magic)),
                       discovery::DiscoveryParseError::invalid_magic,
                       "reject discovery magic");
    suite.expect_false(discovery::DiscoveryMessage::advertisement(
                           1U, 1U, discovery::MachineRole::unspecified,
                           discovery::Capability::none, 0U, 0U, "bad").has_value(),
                       "reject invalid discovery advertisement");

    discovery::LabConfiguration configuration;
    suite.expect_equal(configuration.add_or_update(
                           {1U, "leader", "10.0.0.1", discovery::MachineRole::leader,
                            capabilities, 7000U, 8000U}),
                       discovery::ConfigurationResult::added,
                       "add discovered leader configuration");
    suite.expect_equal(configuration.add_or_update(
                           {2U, "linux-observer", "10.0.0.2",
                            discovery::MachineRole::observer,
                            discovery::Capability::opengl, 0U, 8001U}),
                       discovery::ConfigurationResult::added,
                       "add Linux observer configuration");
    suite.expect_equal(configuration.add_or_update(
                           {3U, "windows-a", "10.0.0.3",
                            discovery::MachineRole::observer,
                            discovery::Capability::opengl, 0U, 8002U}),
                       discovery::ConfigurationResult::added,
                       "add first Windows observer configuration");
    suite.expect_false(configuration.complete(),
                       "three machines do not complete four-computer lab");
    suite.expect_equal(configuration.add_or_update(
                           {4U, "windows-b", "10.0.0.4",
                            discovery::MachineRole::observer,
                            discovery::Capability::opengl, 0U, 8003U}),
                       discovery::ConfigurationResult::added,
                       "add second Windows observer configuration");
    suite.expect_true(configuration.complete(),
                      "validate complete four-computer configuration");
    suite.expect_equal(configuration.observers().size(), std::size_t{3U},
                       "identify three configured observers");
    suite.expect_equal(configuration.leader()->node_id, std::uint64_t{1U},
                       "identify configured leader");
    suite.expect_equal(configuration.add_or_update(
                           {5U, "windows-b", "10.0.0.5",
                            discovery::MachineRole::observer,
                            discovery::Capability::opengl, 0U, 9000U}),
                       discovery::ConfigurationResult::rejected_conflict,
                       "reject duplicate machine identity");

    transport::SocketRuntime runtime;
    auto responder_socket = transport::UdpSocket::bind_ipv4("127.0.0.1", 0U);
    auto client_socket = transport::UdpSocket::bind_ipv4("127.0.0.1", 0U);
    suite.expect_true(std::holds_alternative<transport::UdpSocket>(responder_socket),
                      "bind discovery responder");
    suite.expect_true(std::holds_alternative<transport::UdpSocket>(client_socket),
                      "bind discovery client");
    if (std::holds_alternative<transport::UdpSocket>(responder_socket) &&
        std::holds_alternative<transport::UdpSocket>(client_socket)) {
        const auto port = std::get<transport::UdpSocket>(responder_socket).local_port();
        auto responder_advertisement = discovery::DiscoveryMessage::advertisement(
            1U, 1001U, discovery::MachineRole::leader, capabilities,
            7000U, 8000U, "gpu-desktop");
        discovery::DiscoveryResponder responder{
            std::get<transport::UdpSocket>(std::move(responder_socket)),
            std::move(responder_advertisement.value())};
        std::variant<std::string, discovery::DiscoveryServiceError> response{
            discovery::DiscoveryServiceError::network_error};
        std::thread worker{[&responder, &response] { response = responder.respond_once(); }};
        discovery::DiscoveryClient client{
            std::get<transport::UdpSocket>(std::move(client_socket))};
        const auto machine = client.discover_one("127.0.0.1", port, 1234U);
        worker.join();
        suite.expect_true(std::holds_alternative<std::string>(response),
                          "respond to LAN discovery query");
        suite.expect_true(std::holds_alternative<discovery::DiscoveredMachine>(machine),
                          "discover machine over UDP loopback");
        if (std::holds_alternative<discovery::DiscoveredMachine>(machine)) {
            suite.expect_equal(std::get<discovery::DiscoveredMachine>(machine).address,
                               std::string{"127.0.0.1"},
                               "use packet source as discovered address");
        }
    }
}

}  // namespace silicon_switch::test
