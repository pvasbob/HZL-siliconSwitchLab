#include "switching/mac_table_test.hpp"

#include "silicon_switch/network/mac_address.hpp"
#include "silicon_switch/network/vlan_id.hpp"
#include "silicon_switch/routing/port_id.hpp"
#include "silicon_switch/switching/mac_table.hpp"
#include "test_support.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace silicon_switch::test {
namespace {

network::MacAddress mac(const network::MacAddress::Bytes bytes) {
    return network::MacAddress{bytes};
}

network::VlanId vlan(const std::uint16_t value) {
    return *network::VlanId::create(value);
}

routing::PortId port(const std::uint16_t value) {
    return *routing::PortId::create(value);
}

}  // namespace

void run_mac_table_tests(TestSuite& suite) {
    using namespace std::chrono_literals;

    switching::MacTable table{3U};
    const switching::MacTable::TimePoint start{};
    const auto host = mac({0x02U, 0U, 0U, 0U, 0U, 1U});
    const auto second_host = mac({0x02U, 0U, 0U, 0U, 0U, 2U});
    const auto static_host = mac({0x02U, 0U, 0U, 0U, 0U, 3U});

    suite.expect_equal(table.capacity(), std::size_t{3U},
                       "store MAC table capacity");
    suite.expect_true(table.empty(), "start with empty MAC table");
    suite.expect_false(table.lookup(vlan(10U), host).has_value(),
                       "report MAC table lookup miss");

    suite.expect_equal(
        table.learn(vlan(10U), host, port(1U), start),
        switching::MacTableUpdate::inserted,
        "learn dynamic source MAC");
    const auto learned = table.lookup(vlan(10U), host);
    suite.expect_true(learned.has_value(), "look up learned source MAC");
    if (learned.has_value()) {
        suite.expect_equal(learned->port(), port(1U),
                           "store learned ingress port");
        suite.expect_equal(learned->type(), switching::MacEntryType::dynamic,
                           "classify dynamic MAC entry");
        suite.expect_equal(learned->learned_at(), start,
                           "store dynamic learning timestamp");
    }

    suite.expect_equal(
        table.learn(vlan(10U), host, port(1U), start + 5s),
        switching::MacTableUpdate::refreshed,
        "refresh dynamic MAC on same port");
    suite.expect_equal(table.lookup(vlan(10U), host)->learned_at(), start + 5s,
                       "refresh dynamic MAC timestamp");

    suite.expect_equal(
        table.learn(vlan(10U), host, port(2U), start + 6s),
        switching::MacTableUpdate::moved,
        "detect dynamic MAC movement");
    suite.expect_equal(table.lookup(vlan(10U), host)->port(), port(2U),
                       "update port after MAC movement");

    suite.expect_equal(
        table.learn(vlan(20U), host, port(3U), start),
        switching::MacTableUpdate::inserted,
        "learn identical MAC in another VLAN");
    suite.expect_equal(table.lookup(vlan(10U), host)->port(), port(2U),
                       "preserve first VLAN MAC mapping");
    suite.expect_equal(table.lookup(vlan(20U), host)->port(), port(3U),
                       "isolate identical MAC by VLAN");

    suite.expect_equal(
        table.add_static(vlan(10U), static_host, port(4U)),
        switching::MacTableUpdate::inserted,
        "insert static MAC entry");
    const auto static_entry = table.lookup(vlan(10U), static_host);
    suite.expect_true(static_entry.has_value(), "look up static MAC entry");
    if (static_entry.has_value()) {
        suite.expect_true(static_entry->is_static(), "classify static MAC entry");
        suite.expect_equal(static_entry->port(), port(4U),
                           "store static MAC output port");
    }
    suite.expect_equal(
        table.learn(vlan(10U), static_host, port(5U), start + 10s),
        switching::MacTableUpdate::static_conflict,
        "prevent dynamic movement of static MAC");
    suite.expect_equal(table.lookup(vlan(10U), static_host)->port(), port(4U),
                       "static conflict preserves configured port");
    suite.expect_equal(
        table.learn(vlan(10U), static_host, port(4U), start + 10s),
        switching::MacTableUpdate::refreshed,
        "accept dynamic observation on static port");
    suite.expect_true(table.lookup(vlan(10U), static_host)->is_static(),
                      "dynamic observation preserves static entry type");

    suite.expect_equal(
        table.learn(vlan(10U), second_host, port(1U), start),
        switching::MacTableUpdate::table_full,
        "reject new MAC when table is full");
    suite.expect_false(table.lookup(vlan(10U), second_host).has_value(),
                       "full table does not insert MAC");
    suite.expect_equal(
        table.learn(vlan(10U), host, port(6U), start + 20s),
        switching::MacTableUpdate::moved,
        "full table still updates existing MAC");
    suite.expect_equal(table.size(), std::size_t{3U},
                       "MAC updates preserve table size");

    suite.expect_true(table.remove(vlan(20U), host),
                      "remove VLAN-specific MAC entry");
    suite.expect_false(table.lookup(vlan(20U), host).has_value(),
                       "removed VLAN MAC entry is absent");
    suite.expect_true(table.lookup(vlan(10U), host).has_value(),
                      "removal preserves same MAC in another VLAN");
    suite.expect_false(table.remove(vlan(20U), host),
                       "report removal of missing MAC entry");

    const auto zero = mac({0U, 0U, 0U, 0U, 0U, 0U});
    const auto multicast = mac({0x01U, 0U, 0U, 0U, 0U, 1U});
    const auto broadcast =
        mac({0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU});
    suite.expect_equal(
        table.learn(vlan(10U), zero, port(1U), start),
        switching::MacTableUpdate::rejected,
        "reject unspecified learned source MAC");
    suite.expect_equal(
        table.learn(vlan(10U), multicast, port(1U), start),
        switching::MacTableUpdate::rejected,
        "reject multicast learned source MAC");
    suite.expect_equal(
        table.add_static(vlan(10U), broadcast, port(1U)),
        switching::MacTableUpdate::rejected,
        "reject broadcast static MAC");

    switching::MacTable realtime_table{1U};
    suite.expect_equal(
        realtime_table.learn(vlan(10U), host, port(1U)),
        switching::MacTableUpdate::inserted,
        "learn MAC using current monotonic time");

    bool rejected_zero_capacity = false;
    try {
        const switching::MacTable invalid_table{0U};
        static_cast<void>(invalid_table);
    } catch (const std::invalid_argument&) {
        rejected_zero_capacity = true;
    }
    suite.expect_true(rejected_zero_capacity,
                      "reject zero MAC table capacity");

    switching::MacTable aging_table{4U, 10s};
    suite.expect_equal(aging_table.entry_lifetime(),
                       switching::MacTable::Duration{10s},
                       "store dynamic MAC entry lifetime");
    suite.expect_equal(
        aging_table.learn(vlan(10U), host, port(1U), start),
        switching::MacTableUpdate::inserted,
        "learn MAC for aging");
    suite.expect_equal(
        aging_table.add_static(vlan(10U), static_host, port(2U)),
        switching::MacTableUpdate::inserted,
        "configure static MAC for aging");
    suite.expect_equal(aging_table.expire(start + 9s), std::size_t{0U},
                       "retain dynamic MAC before aging boundary");
    suite.expect_equal(aging_table.expire(start + 10s), std::size_t{1U},
                       "expire dynamic MAC at aging boundary");
    suite.expect_false(aging_table.lookup(vlan(10U), host).has_value(),
                       "aged dynamic MAC is absent");
    suite.expect_true(aging_table.lookup(vlan(10U), static_host).has_value(),
                      "retain static MAC during aging");
    suite.expect_equal(aging_table.expire(start + 100s), std::size_t{0U},
                       "never expire static MAC entry");

    suite.expect_equal(
        aging_table.learn(vlan(10U), host, port(1U), start),
        switching::MacTableUpdate::inserted,
        "relearn dynamic MAC for refresh aging");
    suite.expect_equal(
        aging_table.learn(vlan(10U), host, port(1U), start + 8s),
        switching::MacTableUpdate::refreshed,
        "refresh dynamic MAC aging timestamp");
    suite.expect_equal(aging_table.expire(start + 10s), std::size_t{0U},
                       "refreshed MAC survives original aging boundary");
    suite.expect_equal(aging_table.expire(start + 18s), std::size_t{1U},
                       "refreshed MAC expires at new aging boundary");

    bool rejected_zero_lifetime = false;
    try {
        const switching::MacTable invalid_lifetime_table{
            1U, switching::MacTable::Duration::zero()};
        static_cast<void>(invalid_lifetime_table);
    } catch (const std::invalid_argument&) {
        rejected_zero_lifetime = true;
    }
    suite.expect_true(rejected_zero_lifetime,
                      "reject nonpositive MAC entry lifetime");
}

}  // namespace silicon_switch::test
