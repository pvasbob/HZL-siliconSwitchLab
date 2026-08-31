#pragma once

#include "silicon_switch/simulation/backend.hpp"

#include <memory>
#include <optional>
#include <string_view>
#include <variant>

namespace silicon_switch::simulation {

enum class RuntimeMode { automatic, cpu, cuda, observer };
enum class RuntimeSelectionError { cuda_not_compiled, cuda_device_unavailable };

struct SelectedRuntime {
    RuntimeMode mode;
    std::unique_ptr<SimulationBackend> backend;
};

[[nodiscard]] std::optional<RuntimeMode> parse_runtime_mode(std::string_view value);
[[nodiscard]] std::variant<SelectedRuntime, RuntimeSelectionError> select_runtime(
    RuntimeMode requested, SimulationConfig config = {});

}  // namespace silicon_switch::simulation
