#include "silicon_switch/simulation/backend.hpp"

#include <cmath>
#include <memory>
#include <string>
#include <utility>

namespace silicon_switch::simulation {
namespace {
constexpr float pi = 3.14159265358979323846F;

class CpuBackend final : public SimulationBackend {
public:
    explicit CpuBackend(const SimulationConfig config) : config_{config} {}

    [[nodiscard]] std::string_view name() const noexcept override { return "cpu"; }
    [[nodiscard]] bool accelerated() const noexcept override { return false; }

    [[nodiscard]] visualization::TopologyScene step(const std::uint64_t tick) override {
        visualization::TopologyScene scene;
        scene.upsert_node({1U, 0.0F, 0.0F, 1U, tick * 100U, true, "switch"});
        const auto count = config_.observer_count == 0U ? 1U : config_.observer_count;
        for (std::size_t index = 0U; index < count; ++index) {
            const float phase = 2.0F * pi * static_cast<float>(index) /
                                static_cast<float>(count) +
                                static_cast<float>(tick % 360U) * pi / 1800.0F;
            const auto id = static_cast<std::uint32_t>(index + 2U);
            const auto packets = tick * (static_cast<std::uint64_t>(index) + 1U) * 17U +
                                 config_.seed;
            scene.upsert_node({id, 0.72F * std::cos(phase), 0.72F * std::sin(phase),
                               static_cast<std::uint16_t>(10U + index % 4U), packets,
                               true, "observer-" + std::to_string(index + 1U)});
            scene.upsert_link({1U, id, packets, true});
        }
        return scene;
    }

private:
    SimulationConfig config_;
};
}  // namespace

std::unique_ptr<SimulationBackend> make_cpu_backend(const SimulationConfig config) {
    return std::make_unique<CpuBackend>(config);
}

}  // namespace silicon_switch::simulation
