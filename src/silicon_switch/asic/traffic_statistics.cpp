#include "silicon_switch/asic/traffic_statistics.hpp"

#include <cstddef>
#include <cstdint>

namespace silicon_switch::asic {
namespace {

constexpr std::size_t index(const TrafficCounter counter) noexcept {
    return static_cast<std::size_t>(counter);
}

}  // namespace

std::uint64_t TrafficStatisticsSnapshot::get(
    const TrafficCounter counter) const noexcept {
    return values_[index(counter)];
}

void TrafficStatistics::increment(
    const TrafficCounter counter,
    const std::uint64_t amount) noexcept {
    counters_[index(counter)].fetch_add(amount, std::memory_order_relaxed);
}

std::uint64_t TrafficStatistics::get(
    const TrafficCounter counter) const noexcept {
    return counters_[index(counter)].load(std::memory_order_relaxed);
}

TrafficStatisticsSnapshot TrafficStatistics::snapshot() const noexcept {
    TrafficStatisticsSnapshot result;
    for (std::size_t counter = 0U; counter < counter_count; ++counter) {
        result.values_[counter] =
            counters_[counter].load(std::memory_order_relaxed);
    }
    return result;
}

void TrafficStatistics::reset() noexcept {
    for (auto& counter : counters_) {
        counter.store(0U, std::memory_order_relaxed);
    }
}

}  // namespace silicon_switch::asic
