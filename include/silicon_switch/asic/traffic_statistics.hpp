#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace silicon_switch::asic {

enum class TrafficCounter : std::size_t {
    ingress_packets,
    ingress_bytes,
    egress_packets,
    egress_bytes,
    parse_errors,
    route_misses,
    ttl_expirations,
    vlan_drops,
    neighbor_misses,
    filtered_frames,
    queue_drops,
    fault_drops,
    corrupted_packets,
    resource_exhaustion,
    count,
};

class TrafficStatisticsSnapshot {
public:
    static constexpr std::size_t counter_count =
        static_cast<std::size_t>(TrafficCounter::count);

    [[nodiscard]] std::uint64_t get(TrafficCounter counter) const noexcept;

private:
    friend class TrafficStatistics;
    std::array<std::uint64_t, counter_count> values_{};
};

class TrafficStatistics {
public:
    static constexpr std::size_t counter_count =
        TrafficStatisticsSnapshot::counter_count;

    void increment(TrafficCounter counter, std::uint64_t amount = 1U) noexcept;
    [[nodiscard]] std::uint64_t get(TrafficCounter counter) const noexcept;
    [[nodiscard]] TrafficStatisticsSnapshot snapshot() const noexcept;
    void reset() noexcept;

private:
    std::array<std::atomic<std::uint64_t>, counter_count> counters_{};
};

}  // namespace silicon_switch::asic
