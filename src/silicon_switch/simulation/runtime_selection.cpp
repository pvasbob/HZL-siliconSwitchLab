#include "silicon_switch/simulation/runtime_selection.hpp"

#include <utility>

namespace silicon_switch::simulation {

std::optional<RuntimeMode> parse_runtime_mode(const std::string_view value) {
    if (value == "auto") {
        return RuntimeMode::automatic;
    }
    if (value == "cpu") {
        return RuntimeMode::cpu;
    }
    if (value == "cuda") {
        return RuntimeMode::cuda;
    }
    if (value == "observer") {
        return RuntimeMode::observer;
    }
    return std::nullopt;
}

std::variant<SelectedRuntime, RuntimeSelectionError> select_runtime(
    const RuntimeMode requested, const SimulationConfig config) {
    if (requested == RuntimeMode::observer) {
        return SelectedRuntime{RuntimeMode::observer, nullptr};
    }
    if (requested == RuntimeMode::cpu) {
        return SelectedRuntime{RuntimeMode::cpu, make_cpu_backend(config)};
    }
    if (requested == RuntimeMode::cuda) {
        if (!cuda_backend_compiled()) {
            return RuntimeSelectionError::cuda_not_compiled;
        }
        if (!cuda_device_available()) {
            return RuntimeSelectionError::cuda_device_unavailable;
        }
        return SelectedRuntime{RuntimeMode::cuda, make_cuda_backend(config)};
    }
    if (cuda_backend_compiled() && cuda_device_available()) {
        return SelectedRuntime{RuntimeMode::cuda, make_cuda_backend(config)};
    }
    return SelectedRuntime{RuntimeMode::cpu, make_cpu_backend(config)};
}

}  // namespace silicon_switch::simulation
