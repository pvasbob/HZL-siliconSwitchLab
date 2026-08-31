#include "silicon_switch/discovery/discovery_service.hpp"
#include "silicon_switch/transport/socket_handle.hpp"

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <variant>

namespace discovery = silicon_switch::discovery;
namespace transport = silicon_switch::transport;
namespace {
discovery::Capability parse_capabilities(const std::string& value) {
    auto result = discovery::Capability::none;
    if (value.find("cuda") != std::string::npos) {
        result = result | discovery::Capability::cuda;
    }
    if (value.find("opengl") != std::string::npos) {
        result = result | discovery::Capability::opengl;
    }
    if (value.find("control") != std::string::npos) {
        result = result | discovery::Capability::control_client;
    }
    return result;
}

std::string escaped(const std::string& value) {
    std::string result;
    for (const char character : value) {
        if (character == '"' || character == '\\') {
            result.push_back('\\');
        }
        result.push_back(character);
    }
    return result;
}

int advertise(int argc, char** argv) {
    if (argc != 10) {
        return EXIT_FAILURE;
    }
    const auto role = std::string{argv[5]} == "leader" ? discovery::MachineRole::leader
                                                        : discovery::MachineRole::observer;
    const auto message = discovery::DiscoveryMessage::advertisement(
        1U, std::stoull(argv[4]), role, parse_capabilities(argv[9]),
        static_cast<std::uint16_t>(std::stoul(argv[7])),
        static_cast<std::uint16_t>(std::stoul(argv[8])), argv[6]);
    auto socket = transport::UdpSocket::bind_ipv4(argv[2],
                                                   static_cast<std::uint16_t>(std::stoul(argv[3])));
    if (!message.has_value() || !std::holds_alternative<transport::UdpSocket>(socket)) {
        std::cerr << "invalid advertisement configuration\n";
        return EXIT_FAILURE;
    }
    discovery::DiscoveryResponder responder{
        std::get<transport::UdpSocket>(std::move(socket)), message.value()};
    const auto result = responder.respond_once();
    if (!std::holds_alternative<std::string>(result)) {
        std::cerr << "discovery response failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "responded to " << std::get<std::string>(result) << '\n';
    return EXIT_SUCCESS;
}

int discover(int argc, char** argv) {
    if (argc < 4 || argc > 5) {
        return EXIT_FAILURE;
    }
    auto socket = transport::UdpSocket::bind_ipv4("0.0.0.0", 0U);
    if (!std::holds_alternative<transport::UdpSocket>(socket)) {
        return EXIT_FAILURE;
    }
    discovery::DiscoveryClient client{std::get<transport::UdpSocket>(std::move(socket))};
    static_cast<void>(client.enable_broadcast());
    const auto timeout = argc == 5 ? std::stoll(argv[4]) : 1'000LL;
    static_cast<void>(client.set_receive_timeout(std::chrono::milliseconds{timeout}));
    const auto request_id = static_cast<std::uint64_t>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    const auto found = client.discover_all(
        argv[2], static_cast<std::uint16_t>(std::stoul(argv[3])), request_id);
    if (!std::holds_alternative<std::vector<discovery::DiscoveredMachine>>(found)) {
        return EXIT_FAILURE;
    }
    const auto& machines = std::get<std::vector<discovery::DiscoveredMachine>>(found);
    std::cout << "{\"machines\":[";
    for (std::size_t index = 0U; index < machines.size(); ++index) {
        const auto& machine = machines[index];
        if (index != 0U) {
            std::cout << ',';
        }
        std::cout << "{\"node_id\":" << machine.node_id
                  << ",\"name\":\"" << escaped(machine.name)
                  << "\",\"address\":\"" << escaped(machine.address)
                  << "\",\"role\":\""
                  << (machine.role == discovery::MachineRole::leader ? "leader" : "observer")
                  << "\",\"control_port\":" << machine.control_port
                  << ",\"state_port\":" << machine.state_port << '}';
    }
    std::cout << "]}\n";
    return machines.empty() ? EXIT_FAILURE : EXIT_SUCCESS;
}
}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: silicon_switch_discovery advertise|discover ...\n";
        return EXIT_FAILURE;
    }
    try {
        transport::SocketRuntime runtime;
        const std::string command{argv[1]};
        if (command == "advertise") {
            return advertise(argc, argv);
        }
        if (command == "discover") {
            return discover(argc, argv);
        }
    } catch (const std::exception& error) {
        std::cerr << "discovery error: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
    std::cerr << "usage: silicon_switch_discovery advertise|discover ...\n";
    return EXIT_FAILURE;
}
