#include "silicon_switch/simulation/authoritative_server.hpp"
#include "silicon_switch/simulation/runtime_selection.hpp"
#include "silicon_switch/transport/socket_handle.hpp"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <variant>
#include <vector>

namespace simulation = silicon_switch::simulation;
namespace transport = silicon_switch::transport;
namespace {
std::atomic<bool> running{true};

void stop(const int) { running.store(false); }

std::optional<simulation::ObserverEndpoint> parse_endpoint(const std::string& value) {
    const auto separator = value.rfind(':');
    if (separator == std::string::npos || separator == 0U) {
        return std::nullopt;
    }
    try {
        const auto port = std::stoul(value.substr(separator + 1U));
        if (port == 0U || port > 65'535U) {
            return std::nullopt;
        }
        return simulation::ObserverEndpoint{value.substr(0U, separator),
                                            static_cast<std::uint16_t>(port)};
    } catch (const std::exception&) {
        return std::nullopt;
    }
}
}  // namespace

int main(int argc, char** argv) {
    simulation::RuntimeMode mode = simulation::RuntimeMode::automatic;
    std::vector<simulation::ObserverEndpoint> observers;
    std::uint64_t maximum_ticks = 0U;
    std::uint64_t rate = 30U;

    for (int index = 1; index < argc; ++index) {
        const std::string argument{argv[index]};
        if (argument == "--backend" && index + 1 < argc) {
            const auto parsed = simulation::parse_runtime_mode(argv[++index]);
            if (!parsed.has_value() || parsed.value() == simulation::RuntimeMode::observer) {
                std::cerr << "invalid server backend\n";
                return EXIT_FAILURE;
            }
            mode = parsed.value();
        } else if (argument == "--observer" && index + 1 < argc) {
            const auto endpoint = parse_endpoint(argv[++index]);
            if (!endpoint.has_value()) {
                std::cerr << "invalid observer endpoint\n";
                return EXIT_FAILURE;
            }
            observers.push_back(endpoint.value());
        } else if (argument == "--ticks" && index + 1 < argc) {
            maximum_ticks = std::stoull(argv[++index]);
        } else if (argument == "--hz" && index + 1 < argc) {
            rate = std::stoull(argv[++index]);
            if (rate == 0U || rate > 1'000U) {
                std::cerr << "rate must be between 1 and 1000 Hz\n";
                return EXIT_FAILURE;
            }
        } else {
            std::cerr << "usage: silicon_switch_server [--backend auto|cpu|cuda] "
                         "[--observer IPv4:port] [--ticks count] [--hz rate]\n";
            return EXIT_FAILURE;
        }
    }

    auto selected = simulation::select_runtime(mode);
    if (std::holds_alternative<simulation::RuntimeSelectionError>(selected)) {
        const auto error = std::get<simulation::RuntimeSelectionError>(selected);
        std::cerr << (error == simulation::RuntimeSelectionError::cuda_not_compiled
                          ? "CUDA backend was not compiled\n"
                          : "no usable CUDA device was found\n");
        return EXIT_FAILURE;
    }
    auto runtime = std::get<simulation::SelectedRuntime>(std::move(selected));

    transport::SocketRuntime socket_runtime;
    auto socket_result = transport::UdpSocket::bind_ipv4("0.0.0.0", 0U);
    if (!std::holds_alternative<transport::UdpSocket>(socket_result)) {
        std::cerr << "failed to create UDP publisher socket\n";
        return EXIT_FAILURE;
    }
    const auto session = static_cast<std::uint64_t>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    simulation::AuthoritativeServer server{
        std::move(runtime.backend),
        std::get<transport::UdpSocket>(std::move(socket_result)), session};
    for (auto endpoint : observers) {
        server.add_observer(std::move(endpoint));
    }

    std::signal(SIGINT, stop);
    std::signal(SIGTERM, stop);
    std::cout << "simulation server backend=" << server.backend_name()
              << " observers=" << observers.size() << '\n';
    const auto interval = std::chrono::microseconds{1'000'000U / rate};
    std::uint64_t ticks = 0U;
    while (running.load() && (maximum_ticks == 0U || ticks < maximum_ticks)) {
        const auto started = std::chrono::steady_clock::now();
        const auto timestamp = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(
                started.time_since_epoch()).count());
        const auto published = server.publish_next(timestamp);
        if (!std::holds_alternative<std::size_t>(published)) {
            std::cerr << "failed to publish simulation update\n";
            return EXIT_FAILURE;
        }
        ++ticks;
        std::this_thread::sleep_until(started + interval);
    }
    std::cout << "published " << ticks << " revisions\n";
    return EXIT_SUCCESS;
}
