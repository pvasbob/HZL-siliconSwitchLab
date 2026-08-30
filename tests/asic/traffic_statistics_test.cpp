#include "asic/traffic_statistics_test.hpp"
#include "silicon_switch/asic/traffic_statistics.hpp"
#include "test_support.hpp"
#include <cstdint>
#include <thread>

namespace silicon_switch::test {
void run_traffic_statistics_tests(TestSuite& suite) {
    asic::TrafficStatistics stats;
    suite.expect_equal(stats.get(asic::TrafficCounter::ingress_packets),std::uint64_t{0U},
                       "traffic counters start at zero");
    stats.increment(asic::TrafficCounter::ingress_packets);
    stats.increment(asic::TrafficCounter::ingress_bytes,128U);
    stats.increment(asic::TrafficCounter::route_misses,2U);
    stats.increment(asic::TrafficCounter::ttl_expirations);
    stats.increment(asic::TrafficCounter::vlan_drops,3U);
    stats.increment(asic::TrafficCounter::neighbor_misses);
    stats.increment(asic::TrafficCounter::filtered_frames);
    stats.increment(asic::TrafficCounter::queue_drops,4U);
    stats.increment(asic::TrafficCounter::parse_errors);
    stats.increment(asic::TrafficCounter::resource_exhaustion);
    const auto snapshot=stats.snapshot();
    suite.expect_equal(snapshot.get(asic::TrafficCounter::ingress_packets),std::uint64_t{1U},
                       "snapshot ingress packet counter");
    suite.expect_equal(snapshot.get(asic::TrafficCounter::ingress_bytes),std::uint64_t{128U},
                       "snapshot ingress byte counter");
    suite.expect_equal(snapshot.get(asic::TrafficCounter::route_misses),std::uint64_t{2U},
                       "snapshot route miss counter");
    suite.expect_equal(snapshot.get(asic::TrafficCounter::vlan_drops),std::uint64_t{3U},
                       "snapshot VLAN drop counter");
    suite.expect_equal(snapshot.get(asic::TrafficCounter::queue_drops),std::uint64_t{4U},
                       "snapshot queue drop counter");
    stats.increment(asic::TrafficCounter::ingress_packets);
    suite.expect_equal(snapshot.get(asic::TrafficCounter::ingress_packets),std::uint64_t{1U},
                       "statistics snapshot remains immutable");

    std::thread first{[&stats] { for (int i=0;i<1000;++i) stats.increment(asic::TrafficCounter::egress_packets); }};
    std::thread second{[&stats] { for (int i=0;i<1000;++i) stats.increment(asic::TrafficCounter::egress_packets); }};
    first.join(); second.join();
    suite.expect_equal(stats.get(asic::TrafficCounter::egress_packets),std::uint64_t{2000U},
                       "atomically count concurrent traffic updates");
    stats.reset();
    suite.expect_equal(stats.get(asic::TrafficCounter::ingress_packets),std::uint64_t{0U},
                       "reset packet counter");
    suite.expect_equal(stats.get(asic::TrafficCounter::egress_packets),std::uint64_t{0U},
                       "reset concurrent counter");
    suite.expect_equal(stats.get(asic::TrafficCounter::queue_drops),std::uint64_t{0U},
                       "reset drop counter");
} }
