#include "silicon_switch/simulation/backend.hpp"

namespace silicon_switch::simulation {

std::unique_ptr<SimulationBackend> make_cuda_backend(const SimulationConfig) {
    return nullptr;
}

bool cuda_backend_compiled() noexcept { return false; }
bool cuda_device_available() noexcept { return false; }

}  // namespace silicon_switch::simulation
