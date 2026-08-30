#include "switching/vlan_port_config_test.hpp"

#include "silicon_switch/network/vlan_id.hpp"
#include "silicon_switch/switching/vlan_port_config.hpp"
#include "test_support.hpp"

#include <cstdint>

namespace silicon_switch::test {
namespace {

network::VlanId vlan(const std::uint16_t value) {
    return *network::VlanId::create(value);
}

}  // namespace

void run_vlan_port_config_tests(TestSuite& suite) {
    const auto access = switching::VlanPortConfig::access(vlan(10U));
    suite.expect_true(access.is_access(), "create access-port configuration");
    suite.expect_false(access.is_trunk(), "access port is not trunk port");
    suite.expect_equal(access.mode(), switching::VlanPortMode::access,
                       "classify access-port mode");
    suite.expect_equal(access.access_vlan().value(), vlan(10U),
                       "store access VLAN");
    suite.expect_false(access.native_vlan().has_value(),
                       "access port has no trunk native VLAN");
    suite.expect_true(access.allows(vlan(10U)),
                      "access port allows configured VLAN");
    suite.expect_false(access.allows(vlan(20U)),
                       "access port rejects another VLAN");

    const auto trunk = switching::VlanPortConfig::trunk(
        {vlan(10U), vlan(20U), vlan(30U)}, vlan(10U));
    suite.expect_true(trunk.has_value(), "create trunk-port configuration");
    if (trunk.has_value()) {
        suite.expect_true(trunk->is_trunk(), "classify trunk-port mode");
        suite.expect_false(trunk->access_vlan().has_value(),
                           "trunk port has no access VLAN");
        suite.expect_equal(trunk->native_vlan().value(), vlan(10U),
                           "store trunk native VLAN");
        suite.expect_true(trunk->allows(vlan(20U)),
                          "trunk allows configured tagged VLAN");
        suite.expect_false(trunk->allows(vlan(40U)),
                           "trunk rejects VLAN outside allowed set");
    }

    const auto tagged_only = switching::VlanPortConfig::trunk({vlan(20U)});
    suite.expect_true(tagged_only.has_value(),
                      "create trunk without native VLAN");
    if (tagged_only.has_value()) {
        suite.expect_false(tagged_only->native_vlan().has_value(),
                           "tagged-only trunk omits native VLAN");
    }

    suite.expect_false(switching::VlanPortConfig::trunk({}).has_value(),
                       "reject trunk without allowed VLANs");
    suite.expect_false(
        switching::VlanPortConfig::trunk({vlan(20U)}, vlan(10U)).has_value(),
        "reject native VLAN outside trunk allowed set");
}

}  // namespace silicon_switch::test
