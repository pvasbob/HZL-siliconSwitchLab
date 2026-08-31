#include "simulation/simulation_test.hpp"

#include "silicon_switch/simulation/authoritative_server.hpp"
#include "silicon_switch/simulation/runtime_selection.hpp"
#include "silicon_switch/transport/socket_handle.hpp"
#include "test_support.hpp"

#include <cstdint>
#include <memory>
#include <variant>

namespace silicon_switch::test {

void run_simulation_tests(TestSuite& suite) {
    auto first = simulation::make_cpu_backend({3U, 99U});
    auto second = simulation::make_cpu_backend({3U, 99U});
    suite.expect_equal(first->name(), std::string_view{"cpu"}, "name CPU simulation backend");
    suite.expect_false(first->accelerated(), "CPU simulation is not accelerated");
    const auto first_frame = first->step(7U);
    const auto second_frame = second->step(7U);
    suite.expect_equal(first_frame.serialize_snapshot(), second_frame.serialize_snapshot(),
                       "CPU simulation is deterministic");
    suite.expect_equal(first_frame.nodes().size(), std::size_t{4U},
                       "CPU simulation creates switch and observers");
    suite.expect_equal(first_frame.links().size(), std::size_t{3U},
                       "CPU simulation connects observer links");
    suite.expect_true(first->step(8U).serialize_snapshot() != first_frame.serialize_snapshot(),
                      "CPU simulation advances state");

    suite.expect_equal(simulation::parse_runtime_mode("auto").value(),
                       simulation::RuntimeMode::automatic, "parse automatic runtime mode");
    suite.expect_equal(simulation::parse_runtime_mode("observer").value(),
                       simulation::RuntimeMode::observer, "parse observer runtime mode");
    suite.expect_false(simulation::parse_runtime_mode("invalid").has_value(),
                       "reject unknown runtime mode");
    auto cpu = simulation::select_runtime(simulation::RuntimeMode::cpu, {2U, 1U});
    suite.expect_true(std::holds_alternative<simulation::SelectedRuntime>(cpu),
                      "select CPU runtime explicitly");
    if (std::holds_alternative<simulation::SelectedRuntime>(cpu)) {
        const auto& selected = std::get<simulation::SelectedRuntime>(cpu);
        suite.expect_equal(selected.mode, simulation::RuntimeMode::cpu,
                           "report selected CPU runtime");
        suite.expect_true(selected.backend != nullptr, "construct selected CPU backend");
    }
    const auto observer = simulation::select_runtime(simulation::RuntimeMode::observer);
    suite.expect_true(std::holds_alternative<simulation::SelectedRuntime>(observer),
                      "select observer-only runtime");
    if (std::holds_alternative<simulation::SelectedRuntime>(observer)) {
        suite.expect_true(std::get<simulation::SelectedRuntime>(observer).backend == nullptr,
                          "observer runtime has no simulation backend");
    }
    const auto automatic = simulation::select_runtime(simulation::RuntimeMode::automatic);
    suite.expect_true(std::holds_alternative<simulation::SelectedRuntime>(automatic),
                      "automatically select an available runtime");
    const auto cuda = simulation::select_runtime(simulation::RuntimeMode::cuda);
    const bool cuda_selected = std::holds_alternative<simulation::SelectedRuntime>(cuda);
    suite.expect_equal(cuda_selected,
                       simulation::cuda_backend_compiled() &&
                           simulation::cuda_device_available(),
                       "select CUDA only when compiled device is operational");

    auto socket_result = transport::UdpSocket::bind_ipv4("127.0.0.1", 0U);
    suite.expect_true(std::holds_alternative<transport::UdpSocket>(socket_result),
                      "create authoritative publisher socket");
    if (!std::holds_alternative<transport::UdpSocket>(socket_result)) {
        return;
    }
    simulation::AuthoritativeServer server{
        simulation::make_cpu_backend({2U, 4U}),
        std::get<transport::UdpSocket>(std::move(socket_result)), 123U, 3U};
    suite.expect_equal(server.backend_name(), std::string_view{"cpu"},
                       "server reports simulation backend");
    const auto snapshot = server.next_update(1'000U);
    suite.expect_true(std::holds_alternative<transport::StateUpdate>(snapshot),
                      "server creates initial state update");
    if (std::holds_alternative<transport::StateUpdate>(snapshot)) {
        const auto& update = std::get<transport::StateUpdate>(snapshot);
        suite.expect_equal(update.type(), transport::StateUpdateType::snapshot,
                           "server starts with full snapshot");
        suite.expect_equal(update.revision(), std::uint64_t{1U},
                           "server numbers first revision");
    }
    const auto delta = server.next_update(2'000U);
    suite.expect_equal(std::get<transport::StateUpdate>(delta).type(),
                       transport::StateUpdateType::delta,
                       "server publishes incremental update");
    const auto periodic_snapshot = server.next_update(3'000U);
    suite.expect_equal(std::get<transport::StateUpdate>(periodic_snapshot).type(),
                       transport::StateUpdateType::snapshot,
                       "server periodically republishes snapshot");
    const auto published = server.publish_next(4'000U);
    suite.expect_equal(std::get<std::size_t>(published), std::size_t{0U},
                       "server supports zero connected observers");
}

}  // namespace silicon_switch::test
