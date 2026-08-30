#include "routing/arp_cache_test.hpp"

#include "silicon_switch/network/ipv4_address.hpp"
#include "silicon_switch/network/mac_address.hpp"
#include "silicon_switch/routing/arp_cache.hpp"
#include "test_support.hpp"

#include <chrono>
#include <cstdint>
#include <stdexcept>

namespace silicon_switch::test {
namespace {

network::Ipv4Address ip(const std::uint32_t value) {
    return network::Ipv4Address{value};
}

network::MacAddress mac(const network::MacAddress::Bytes bytes) {
    return network::MacAddress{bytes};
}

}  // namespace

void run_arp_cache_tests(TestSuite& suite) {
    routing::ArpCache cache;
    const auto address = ip(0xC0A80101U);
    const auto original_mac =
        mac({0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U});
    const auto replacement_mac =
        mac({0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U});

    suite.expect_true(cache.empty(), "start with empty ARP cache");
    suite.expect_equal(cache.size(), std::size_t{0U},
                       "report empty ARP cache size");
    suite.expect_false(cache.lookup(address).has_value(),
                       "report ARP cache lookup miss");

    suite.expect_equal(
        cache.add_or_replace(address, original_mac),
        routing::ArpCacheUpdate::inserted,
        "insert ARP cache entry");
    suite.expect_false(cache.empty(), "report nonempty ARP cache");
    suite.expect_equal(cache.size(), std::size_t{1U},
                       "count inserted ARP cache entry");
    suite.expect_equal(cache.lookup(address).value(), original_mac,
                       "look up ARP cache mapping");

    suite.expect_equal(
        cache.add_or_replace(address, replacement_mac),
        routing::ArpCacheUpdate::replaced,
        "replace ARP cache entry");
    suite.expect_equal(cache.size(), std::size_t{1U},
                       "replacement preserves ARP cache size");
    suite.expect_equal(cache.lookup(address).value(), replacement_mac,
                       "look up replacement ARP mapping");

    const auto second_address = ip(0xC0A80102U);
    suite.expect_equal(
        cache.add_or_replace(second_address, original_mac),
        routing::ArpCacheUpdate::inserted,
        "insert second ARP cache entry");
    suite.expect_equal(cache.size(), std::size_t{2U},
                       "count multiple ARP cache entries");

    suite.expect_true(cache.remove(address), "remove ARP cache entry");
    suite.expect_false(cache.lookup(address).has_value(),
                       "removed ARP cache entry is absent");
    suite.expect_false(cache.remove(address),
                       "report removal of missing ARP cache entry");
    suite.expect_equal(cache.size(), std::size_t{1U},
                       "removal decreases ARP cache size");

    const auto unspecified_mac = mac({0U, 0U, 0U, 0U, 0U, 0U});
    const auto multicast_mac = mac({0x01U, 0U, 0U, 0U, 0U, 1U});
    const auto broadcast_mac =
        mac({0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU});
    suite.expect_equal(
        cache.add_or_replace(ip(0U), original_mac),
        routing::ArpCacheUpdate::rejected,
        "reject unspecified ARP cache IPv4 address");
    suite.expect_equal(
        cache.add_or_replace(ip(0xE0000001U), original_mac),
        routing::ArpCacheUpdate::rejected,
        "reject multicast ARP cache IPv4 address");
    suite.expect_equal(
        cache.add_or_replace(ip(0xFFFFFFFFU), original_mac),
        routing::ArpCacheUpdate::rejected,
        "reject broadcast ARP cache IPv4 address");
    suite.expect_equal(
        cache.add_or_replace(ip(0xC0A80103U), unspecified_mac),
        routing::ArpCacheUpdate::rejected,
        "reject unspecified ARP cache MAC address");
    suite.expect_equal(
        cache.add_or_replace(ip(0xC0A80103U), multicast_mac),
        routing::ArpCacheUpdate::rejected,
        "reject multicast ARP cache MAC address");
    suite.expect_equal(
        cache.add_or_replace(ip(0xC0A80103U), broadcast_mac),
        routing::ArpCacheUpdate::rejected,
        "reject broadcast ARP cache MAC address");
    suite.expect_equal(cache.size(), std::size_t{1U},
                       "rejected mappings do not change ARP cache");

    using namespace std::chrono_literals;
    const routing::ArpCache::TimePoint start{};
    routing::ArpCache aging_cache{10s};
    suite.expect_equal(aging_cache.entry_lifetime(),
                       routing::ArpCache::Duration{10s},
                       "store configured ARP cache lifetime");
    suite.expect_equal(
        aging_cache.add_or_replace(address, original_mac, start),
        routing::ArpCacheUpdate::inserted,
        "timestamp learned ARP cache entry");
    suite.expect_equal(
        aging_cache.lookup(address, start + 9s).value(), original_mac,
        "retain ARP entry immediately before expiration");
    suite.expect_false(
        aging_cache.lookup(address, start + 10s).has_value(),
        "hide ARP entry exactly at expiration boundary");
    suite.expect_equal(aging_cache.size(), std::size_t{1U},
                       "timed lookup does not mutate ARP cache");
    suite.expect_equal(aging_cache.expire(start + 10s), std::size_t{1U},
                       "expire ARP entry at lifetime boundary");
    suite.expect_true(aging_cache.empty(),
                      "aging pass removes expired ARP entry");
    suite.expect_equal(aging_cache.expire(start + 20s), std::size_t{0U},
                       "empty aging pass reports no removals");

    suite.expect_equal(
        aging_cache.add_or_replace(address, original_mac, start),
        routing::ArpCacheUpdate::inserted,
        "reinsert ARP entry for refresh test");
    suite.expect_equal(
        aging_cache.add_or_replace(address, replacement_mac, start + 8s),
        routing::ArpCacheUpdate::replaced,
        "refresh ARP timestamp during replacement");
    suite.expect_equal(
        aging_cache.lookup(address, start + 10s).value(), replacement_mac,
        "refreshed ARP entry survives original expiration time");
    suite.expect_equal(aging_cache.expire(start + 17s), std::size_t{0U},
                       "retain refreshed ARP entry before new boundary");
    suite.expect_equal(aging_cache.expire(start + 18s), std::size_t{1U},
                       "expire refreshed ARP entry at new boundary");

    routing::ArpCache staggered_cache{10s};
    suite.expect_equal(
        staggered_cache.add_or_replace(address, original_mac, start),
        routing::ArpCacheUpdate::inserted,
        "insert older staggered ARP entry");
    suite.expect_equal(
        staggered_cache.add_or_replace(
            second_address, replacement_mac, start + 5s),
        routing::ArpCacheUpdate::inserted,
        "insert newer staggered ARP entry");
    suite.expect_equal(staggered_cache.expire(start + 10s), std::size_t{1U},
                       "aging pass removes only expired entries");
    suite.expect_equal(
        staggered_cache.lookup(second_address, start + 10s).value(),
        replacement_mac,
        "aging pass retains unexpired entries");

    bool rejected_zero_lifetime = false;
    try {
        const routing::ArpCache invalid_cache{routing::ArpCache::Duration::zero()};
        static_cast<void>(invalid_cache);
    } catch (const std::invalid_argument&) {
        rejected_zero_lifetime = true;
    }
    suite.expect_true(rejected_zero_lifetime,
                      "reject nonpositive ARP cache lifetime");
}

}  // namespace silicon_switch::test
