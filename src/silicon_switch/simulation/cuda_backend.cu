#include "silicon_switch/simulation/backend.hpp"

#include <cuda_runtime.h>

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace silicon_switch::simulation {
namespace {
constexpr float pi = 3.14159265358979323846F;

__global__ void update_observers(float* positions,
                                 unsigned long long* packets,
                                 const std::size_t count,
                                 const std::uint64_t tick,
                                 const std::uint64_t seed) {
    const auto index = static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    if (index >= count) {
        return;
    }
    const float phase = 2.0F * pi * static_cast<float>(index) /
                        static_cast<float>(count) +
                        static_cast<float>(tick % 360U) * pi / 1800.0F;
    positions[index * 2U] = 0.72F * cosf(phase);
    positions[index * 2U + 1U] = 0.72F * sinf(phase);
    packets[index] = tick * (static_cast<unsigned long long>(index) + 1ULL) * 17ULL + seed;
}

void require_cuda(const cudaError_t result, const char* operation) {
    if (result != cudaSuccess) {
        throw std::runtime_error{std::string{operation} + ": " + cudaGetErrorString(result)};
    }
}

class CudaBackend final : public SimulationBackend {
public:
    explicit CudaBackend(const SimulationConfig config)
        : config_{config}, count_{config.observer_count == 0U ? 1U : config.observer_count} {
        require_cuda(cudaMalloc(&device_positions_, count_ * 2U * sizeof(float)),
                     "cudaMalloc positions");
        try {
            require_cuda(cudaMalloc(&device_packets_, count_ * sizeof(unsigned long long)),
                         "cudaMalloc counters");
        } catch (...) {
            cudaFree(device_positions_);
            throw;
        }
    }

    ~CudaBackend() override {
        cudaFree(device_packets_);
        cudaFree(device_positions_);
    }

    [[nodiscard]] std::string_view name() const noexcept override { return "cuda"; }
    [[nodiscard]] bool accelerated() const noexcept override { return true; }

    [[nodiscard]] visualization::TopologyScene step(const std::uint64_t tick) override {
        constexpr unsigned int block_size = 256U;
        const auto block_count = static_cast<unsigned int>((count_ + block_size - 1U) /
                                                           block_size);
        update_observers<<<block_count, block_size>>>(device_positions_, device_packets_, count_,
                                                     tick, config_.seed);
        require_cuda(cudaGetLastError(), "CUDA simulation kernel launch");
        std::vector<float> positions(count_ * 2U);
        std::vector<unsigned long long> packets(count_);
        require_cuda(cudaMemcpy(positions.data(), device_positions_, positions.size() * sizeof(float),
                                cudaMemcpyDeviceToHost), "copy CUDA positions");
        require_cuda(cudaMemcpy(packets.data(), device_packets_,
                                packets.size() * sizeof(unsigned long long),
                                cudaMemcpyDeviceToHost), "copy CUDA counters");

        visualization::TopologyScene scene;
        scene.upsert_node({1U, 0.0F, 0.0F, 1U, tick * 100U, true, "switch"});
        for (std::size_t index = 0U; index < count_; ++index) {
            const auto id = static_cast<std::uint32_t>(index + 2U);
            const auto packet_count = static_cast<std::uint64_t>(packets[index]);
            scene.upsert_node({id, positions[index * 2U], positions[index * 2U + 1U],
                               static_cast<std::uint16_t>(10U + index % 4U), packet_count,
                               true, "observer-" + std::to_string(index + 1U)});
            scene.upsert_link({1U, id, packet_count, true});
        }
        return scene;
    }

private:
    SimulationConfig config_;
    std::size_t count_;
    float* device_positions_{nullptr};
    unsigned long long* device_packets_{nullptr};
};
}  // namespace

std::unique_ptr<SimulationBackend> make_cuda_backend(const SimulationConfig config) {
    return std::make_unique<CudaBackend>(config);
}

bool cuda_backend_compiled() noexcept { return true; }

bool cuda_device_available() noexcept {
    int count = 0;
    return cudaGetDeviceCount(&count) == cudaSuccess && count > 0;
}

}  // namespace silicon_switch::simulation
