#pragma once

#include "silicon_switch/visualization/topology.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>

namespace silicon_switch::simulation {

struct SimulationConfig {
    std::size_t observer_count{3U};
    std::uint64_t seed{1U};
};

class SimulationBackend {
public:
    virtual ~SimulationBackend() = default;
    [[nodiscard]] virtual std::string_view name() const noexcept = 0;
    [[nodiscard]] virtual bool accelerated() const noexcept = 0;
    [[nodiscard]] virtual visualization::TopologyScene step(std::uint64_t tick) = 0;
};

[[nodiscard]] std::unique_ptr<SimulationBackend> make_cpu_backend(SimulationConfig config);
[[nodiscard]] std::unique_ptr<SimulationBackend> make_cuda_backend(SimulationConfig config);
[[nodiscard]] bool cuda_backend_compiled() noexcept;
[[nodiscard]] bool cuda_device_available() noexcept;

}  // namespace silicon_switch::simulation
