#include "silicon_switch/asic/bounded_queue.hpp"
#include "silicon_switch/asic/software_asic.hpp"
#include "silicon_switch/network/ethernet_frame.hpp"
#include "silicon_switch/network/mac_address.hpp"
#include "silicon_switch/network/vlan_id.hpp"
#include "silicon_switch/routing/port_id.hpp"
#include "silicon_switch/simulation/runtime_selection.hpp"
#include "silicon_switch/switching/virtual_port.hpp"
#include "silicon_switch/switching/vlan_port_config.hpp"
#include "silicon_switch/transport/udp.hpp"
#include "silicon_switch/visualization/topology_renderer.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace asic = silicon_switch::asic;
namespace network = silicon_switch::network;
namespace routing = silicon_switch::routing;
namespace simulation = silicon_switch::simulation;
namespace switching = silicon_switch::switching;
namespace transport = silicon_switch::transport;
namespace visualization = silicon_switch::visualization;
namespace {
using Clock = std::chrono::steady_clock;

std::optional<std::uint64_t> number(const std::string& value) {
    try {
        std::size_t parsed = 0U;
        const auto result = std::stoull(value, &parsed);
        return parsed == value.size() ? std::optional<std::uint64_t>{result} : std::nullopt;
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

std::optional<std::string> option(int argc, char** argv, const std::string& name) {
    for (int index = 2; index + 1 < argc; ++index) {
        if (argv[index] == name) {
            return argv[index + 1];
        }
    }
    return std::nullopt;
}

std::uint64_t numeric_option(int argc, char** argv, const std::string& name,
                             const std::uint64_t fallback) {
    const auto value = option(argc, argv, name);
    if (!value.has_value()) {
        return fallback;
    }
    const auto parsed = number(value.value());
    return parsed.has_value() ? parsed.value() : 0U;
}

double seconds(const Clock::duration duration) {
    return std::chrono::duration<double>(duration).count();
}

double percentile(std::vector<double> values, const double quantile) {
    if (values.empty()) {
        return 0.0;
    }
    std::sort(values.begin(), values.end());
    const auto index = static_cast<std::size_t>(
        std::ceil(quantile * static_cast<double>(values.size())) - 1.0);
    return values[std::min(index, values.size() - 1U)];
}

void json_number(const char* name, const double value, const bool comma = true) {
    std::cout << '"' << name << "\":" << std::fixed << std::setprecision(3) << value;
    if (comma) {
        std::cout << ',';
    }
}

int simulation_benchmark(int argc, char** argv) {
    const auto iterations = numeric_option(argc, argv, "--iterations", 10'000U);
    const auto observers = numeric_option(argc, argv, "--observers", 3U);
    const auto backend_name = option(argc, argv, "--backend").value_or("cpu");
    const auto mode = simulation::parse_runtime_mode(backend_name);
    if (iterations == 0U || observers == 0U || !mode.has_value() ||
        mode.value() == simulation::RuntimeMode::observer) {
        return EXIT_FAILURE;
    }
    auto selected = simulation::select_runtime(
        mode.value(), {static_cast<std::size_t>(observers), 1U});
    if (!std::holds_alternative<simulation::SelectedRuntime>(selected)) {
        std::cerr << "requested simulation backend is unavailable\n";
        return EXIT_FAILURE;
    }
    auto runtime = std::get<simulation::SelectedRuntime>(std::move(selected));
    for (std::uint64_t tick = 1U; tick <= 20U; ++tick) {
        static_cast<void>(runtime.backend->step(tick));
    }
    std::uint64_t serialized_bytes = 0U;
    const auto started = Clock::now();
    for (std::uint64_t tick = 1U; tick <= iterations; ++tick) {
        serialized_bytes += runtime.backend->step(tick).serialize_snapshot().size();
    }
    const auto elapsed = seconds(Clock::now() - started);
    std::cout << "{\"benchmark\":\"simulation\",\"backend\":\""
              << runtime.backend->name() << "\",\"iterations\":" << iterations << ',';
    json_number("elapsed_seconds", elapsed);
    json_number("steps_per_second", static_cast<double>(iterations) / elapsed);
    json_number("serialized_mib_per_second",
                static_cast<double>(serialized_bytes) / elapsed / 1'048'576.0, false);
    std::cout << "}\n";
    return EXIT_SUCCESS;
}

routing::PortId port(const std::uint16_t value) {
    return routing::PortId::create(value).value();
}
network::VlanId vlan(const std::uint16_t value) {
    return network::VlanId::create(value).value();
}
network::MacAddress mac(const std::uint8_t suffix) {
    return network::MacAddress{{0x02U, 0U, 0U, 0U, 0U, suffix}};
}
switching::VirtualPort virtual_port(const std::uint16_t id) {
    return switching::VirtualPort::create(
        port(id), mac(0xFEU), switching::PortSpeed::gbps_10, 1500U,
        switching::VlanPortConfig::access(vlan(10U))).value();
}

int forwarding_benchmark(int argc, char** argv) {
    const auto iterations = numeric_option(argc, argv, "--iterations", 100'000U);
    const auto payload_size = numeric_option(argc, argv, "--payload", 512U);
    if (iterations == 0U || payload_size > 1'400U) {
        return EXIT_FAILURE;
    }
    asic::SoftwareAsic device;
    if (device.create_vlan(vlan(10U)) != asic::AsicStatus::success ||
        device.create_port(virtual_port(1U), 1U) != asic::AsicStatus::success ||
        device.create_port(virtual_port(2U), 1U) != asic::AsicStatus::success ||
        device.set_port_state(port(1U), true, true) != asic::AsicStatus::success ||
        device.set_port_state(port(2U), true, true) != asic::AsicStatus::success ||
        device.add_vlan_member(vlan(10U), port(1U)) != asic::AsicStatus::success ||
        device.add_vlan_member(vlan(10U), port(2U)) != asic::AsicStatus::success) {
        return EXIT_FAILURE;
    }
    const auto broadcast = network::MacAddress{{0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU}};
    const auto learn = network::EthernetFrame::create(
        broadcast, mac(2U), network::EtherType::ipv4, {1U}).value().serialize();
    static_cast<void>(device.process_packet(port(2U), learn, switching::MacTable::TimePoint{}));
    static_cast<void>(device.dequeue_packet(port(1U)));
    std::vector<std::uint8_t> payload(static_cast<std::size_t>(payload_size), 0x5AU);
    const auto frame = network::EthernetFrame::create(
        mac(2U), mac(1U), network::EtherType::ipv4, std::move(payload)).value().serialize();

    std::uint64_t completed = 0U;
    const auto started = Clock::now();
    for (std::uint64_t index = 0U; index < iterations; ++index) {
        const auto result = device.process_packet(port(1U), frame,
                                                  switching::MacTable::TimePoint{});
        if (result.disposition == asic::PacketDisposition::switched &&
            device.dequeue_packet(port(2U)).has_value()) {
            ++completed;
        }
    }
    const auto elapsed = seconds(Clock::now() - started);
    std::cout << "{\"benchmark\":\"forwarding\",\"packets\":" << completed << ',';
    json_number("elapsed_seconds", elapsed);
    json_number("packets_per_second", static_cast<double>(completed) / elapsed);
    json_number("mib_per_second",
                static_cast<double>(completed * frame.size()) / elapsed / 1'048'576.0,
                false);
    std::cout << "}\n";
    return completed == iterations ? EXIT_SUCCESS : EXIT_FAILURE;
}

int render_benchmark(int argc, char** argv) {
    const auto iterations = numeric_option(argc, argv, "--iterations", 100'000U);
    const auto nodes = numeric_option(argc, argv, "--nodes", 100U);
    if (iterations == 0U || nodes == 0U) {
        return EXIT_FAILURE;
    }
    auto backend = simulation::make_cpu_backend({static_cast<std::size_t>(nodes), 1U});
    const auto scene = backend->step(1U);
    visualization::TopologyRenderer renderer;
    std::uint64_t vertices = 0U;
    const auto started = Clock::now();
    for (std::uint64_t index = 0U; index < iterations; ++index) {
        const auto frame = renderer.build_frame(scene);
        vertices += frame.nodes.size() + frame.links.size();
    }
    const auto elapsed = seconds(Clock::now() - started);
    std::cout << "{\"benchmark\":\"render-preparation\",\"iterations\":"
              << iterations << ",\"vertices\":" << vertices << ',';
    json_number("elapsed_seconds", elapsed);
    json_number("frames_per_second", static_cast<double>(iterations) / elapsed, false);
    std::cout << "}\n";
    return EXIT_SUCCESS;
}

int queue_benchmark(int argc, char** argv) {
    const auto iterations = numeric_option(argc, argv, "--iterations", 1'000'000U);
    const auto capacity = numeric_option(argc, argv, "--capacity", 1'024U);
    if (iterations == 0U || capacity == 0U) {
        return EXIT_FAILURE;
    }
    asic::BoundedQueue<std::uint64_t> queue{static_cast<std::size_t>(capacity)};
    const auto started = Clock::now();
    for (std::uint64_t index = 0U; index < iterations; ++index) {
        static_cast<void>(queue.try_enqueue(index));
        static_cast<void>(queue.try_dequeue());
    }
    const auto elapsed = seconds(Clock::now() - started);
    std::uint64_t full = 0U;
    for (std::uint64_t index = 0U; index <= capacity; ++index) {
        if (queue.try_enqueue(index) == asic::QueueEnqueueResult::full) {
            ++full;
        }
    }
    std::cout << "{\"benchmark\":\"queue\",\"iterations\":" << iterations
              << ",\"capacity\":" << capacity << ",\"full_events\":" << full << ',';
    json_number("operations_per_second", 2.0 * static_cast<double>(iterations) / elapsed,
                false);
    std::cout << "}\n";
    return full == 1U ? EXIT_SUCCESS : EXIT_FAILURE;
}

int udp_server(int argc, char** argv) {
    const auto address = option(argc, argv, "--listen").value_or("0.0.0.0");
    const auto port_value = numeric_option(argc, argv, "--port", 0U);
    const auto packets = numeric_option(argc, argv, "--packets", 1'000U);
    const auto timeout = numeric_option(argc, argv, "--timeout-ms", 10'000U);
    if (port_value == 0U || port_value > 65'535U || packets == 0U) {
        return EXIT_FAILURE;
    }
    auto bound = transport::UdpSocket::bind_ipv4(address, static_cast<std::uint16_t>(port_value));
    if (!std::holds_alternative<transport::UdpSocket>(bound)) {
        return EXIT_FAILURE;
    }
    auto socket = std::get<transport::UdpSocket>(std::move(bound));
    static_cast<void>(socket.set_receive_timeout(std::chrono::milliseconds{timeout}));
    std::uint64_t echoed = 0U;
    std::uint64_t bytes = 0U;
    const auto started = Clock::now();
    while (echoed < packets) {
        auto received = socket.receive(65'507U);
        if (!std::holds_alternative<transport::UdpDatagram>(received)) {
            break;
        }
        auto datagram = std::get<transport::UdpDatagram>(std::move(received));
        bytes += datagram.payload.size();
        if (!std::holds_alternative<std::size_t>(socket.send_to_ipv4(
                datagram.source_address, datagram.source_port, datagram.payload))) {
            break;
        }
        ++echoed;
    }
    const auto elapsed = seconds(Clock::now() - started);
    std::cout << "{\"benchmark\":\"udp-echo-server\",\"packets\":" << echoed
              << ",\"bytes\":" << bytes << ',';
    json_number("elapsed_seconds", elapsed, false);
    std::cout << "}\n";
    return echoed == packets ? EXIT_SUCCESS : EXIT_FAILURE;
}

int udp_client(int argc, char** argv) {
    const auto target = option(argc, argv, "--target");
    const auto port_value = numeric_option(argc, argv, "--port", 0U);
    const auto packets = numeric_option(argc, argv, "--packets", 1'000U);
    const auto payload_size = numeric_option(argc, argv, "--payload", 1'024U);
    const auto timeout = numeric_option(argc, argv, "--timeout-ms", 1'000U);
    if (!target.has_value() || port_value == 0U || port_value > 65'535U ||
        packets == 0U || payload_size == 0U || payload_size > 65'507U) {
        return EXIT_FAILURE;
    }
    auto bound = transport::UdpSocket::bind_ipv4("0.0.0.0", 0U);
    if (!std::holds_alternative<transport::UdpSocket>(bound)) {
        return EXIT_FAILURE;
    }
    auto socket = std::get<transport::UdpSocket>(std::move(bound));
    static_cast<void>(socket.set_receive_timeout(std::chrono::milliseconds{timeout}));
    std::vector<std::uint8_t> payload(static_cast<std::size_t>(payload_size), 0xA5U);
    std::vector<double> latencies_ms;
    std::uint64_t bytes = 0U;
    const auto overall_started = Clock::now();
    for (std::uint64_t sequence = 0U; sequence < packets; ++sequence) {
        for (std::size_t index = 0U; index < sizeof(sequence); ++index) {
            payload[index % payload.size()] = static_cast<std::uint8_t>(
                sequence >> static_cast<unsigned int>(index * 8U));
        }
        const auto started = Clock::now();
        if (!std::holds_alternative<std::size_t>(socket.send_to_ipv4(
                target.value(), static_cast<std::uint16_t>(port_value), payload))) {
            continue;
        }
        const auto response = socket.receive(payload.size());
        if (std::holds_alternative<transport::UdpDatagram>(response) &&
            std::get<transport::UdpDatagram>(response).payload == payload) {
            latencies_ms.push_back(
                std::chrono::duration<double, std::milli>(Clock::now() - started).count());
            bytes += payload.size() * 2U;
        }
    }
    const auto elapsed = seconds(Clock::now() - overall_started);
    const auto received = static_cast<std::uint64_t>(latencies_ms.size());
    const auto loss = 100.0 * static_cast<double>(packets - received) /
                      static_cast<double>(packets);
    const auto average = latencies_ms.empty()
                             ? 0.0
                             : std::accumulate(latencies_ms.begin(), latencies_ms.end(), 0.0) /
                                   static_cast<double>(latencies_ms.size());
    std::cout << "{\"benchmark\":\"udp-round-trip\",\"target\":\""
              << target.value() << "\",\"sent\":" << packets
              << ",\"received\":" << received << ',';
    json_number("loss_percent", loss);
    json_number("rtt_average_ms", average);
    json_number("rtt_p50_ms", percentile(latencies_ms, 0.50));
    json_number("rtt_p95_ms", percentile(latencies_ms, 0.95));
    json_number("rtt_p99_ms", percentile(latencies_ms, 0.99));
    json_number("effective_mib_per_second",
                static_cast<double>(bytes) / elapsed / 1'048'576.0, false);
    std::cout << "}\n";
    return received > 0U ? EXIT_SUCCESS : EXIT_FAILURE;
}
}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: silicon_switch_benchmark "
                     "simulation|forwarding|render|queue|udp-server|udp-client [options]\n";
        return EXIT_FAILURE;
    }
    const std::string command{argv[1]};
    if (command == "simulation") return simulation_benchmark(argc, argv);
    if (command == "forwarding") return forwarding_benchmark(argc, argv);
    if (command == "render") return render_benchmark(argc, argv);
    if (command == "queue") return queue_benchmark(argc, argv);
    if (command == "udp-server") return udp_server(argc, argv);
    if (command == "udp-client") return udp_client(argc, argv);
    return EXIT_FAILURE;
}
