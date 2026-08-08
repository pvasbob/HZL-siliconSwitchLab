#include "network/vlan_test.hpp"

#include "silicon_switch/network/vlan_id.hpp"
#include "silicon_switch/network/vlan_tag.hpp"
#include "test_support.hpp"

#include <cstdint>
#include <optional>

namespace silicon_switch::test {

void run_vlan_tests(TestSuite& suite) {
    using network::VlanId;
    using network::VlanTag;

    constexpr auto vlan_id = VlanId::create(100U);
    static_assert(vlan_id.has_value());
    static_assert(vlan_id->value() == 100U);
    suite.expect_true(vlan_id.has_value(), "create valid VLAN identifier");

    suite.expect_false(
        VlanId::create(0U).has_value(),
        "reject reserved zero VLAN identifier");
    suite.expect_false(
        VlanId::create(4095U).has_value(),
        "reject reserved maximum VLAN identifier");

    constexpr auto tag = VlanTag::create(*vlan_id, 5U, true);
    static_assert(tag.has_value());
    static_assert(tag->vlan_id() == *vlan_id);
    static_assert(tag->priority() == 5U);
    static_assert(tag->drop_eligible());
    static_assert(tag->tag_control_information() == 0xB064U);
    suite.expect_true(tag.has_value(), "create valid VLAN tag");

    suite.expect_false(
        VlanTag::create(*vlan_id, 8U).has_value(),
        "reject invalid VLAN priority");

    constexpr auto decoded_tag =
        VlanTag::from_tag_control_information(0xB064U);
    static_assert(decoded_tag == tag);
    suite.expect_equal(
        decoded_tag,
        tag,
        "round-trip VLAN tag control information");

    suite.expect_false(
        VlanTag::from_tag_control_information(0x0000U).has_value(),
        "reject unsupported priority-only VLAN tag");
    suite.expect_false(
        VlanTag::from_tag_control_information(0x0FFFU).has_value(),
        "reject reserved VLAN tag identifier");
}

}  // namespace silicon_switch::test
