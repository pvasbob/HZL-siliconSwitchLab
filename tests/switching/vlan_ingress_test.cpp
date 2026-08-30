#include "switching/vlan_ingress_test.hpp"

#include "silicon_switch/network/ether_type.hpp"
#include "silicon_switch/network/ethernet_frame.hpp"
#include "silicon_switch/network/mac_address.hpp"
#include "silicon_switch/network/vlan_id.hpp"
#include "silicon_switch/network/vlan_tag.hpp"
#include "silicon_switch/switching/vlan_ingress.hpp"
#include "silicon_switch/switching/vlan_port_config.hpp"
#include "test_support.hpp"

#include <cstdint>
#include <optional>
#include <variant>
#include <vector>

namespace silicon_switch::test {
namespace {

network::VlanId vlan(const std::uint16_t value) {
    return *network::VlanId::create(value);
}

network::EthernetFrame frame(
    const std::optional<network::VlanId> frame_vlan = std::nullopt) {
    std::optional<network::VlanTag> tag;
    if (frame_vlan.has_value()) {
        tag = network::VlanTag::create(*frame_vlan).value();
    }
    return *network::EthernetFrame::create(
        network::MacAddress{{0x02U, 0U, 0U, 0U, 0U, 1U}},
        network::MacAddress{{0x02U, 0U, 0U, 0U, 0U, 2U}},
        network::EtherType::ipv4,
        std::vector<std::uint8_t>{0x01U},
        tag);
}

const switching::AcceptedVlanIngress* accepted(
    const switching::VlanIngressResult& result) {
    return std::get_if<switching::AcceptedVlanIngress>(&result);
}

const switching::DroppedVlanIngress* dropped(
    const switching::VlanIngressResult& result) {
    return std::get_if<switching::DroppedVlanIngress>(&result);
}

}  // namespace

void run_vlan_ingress_tests(TestSuite& suite) {
    const auto untagged = frame();
    const auto tagged_ten = frame(vlan(10U));
    const auto tagged_twenty = frame(vlan(20U));
    const auto original_tagged_bytes = tagged_ten.serialize();

    const auto access = switching::VlanPortConfig::access(vlan(10U));
    const auto access_result =
        switching::classify_vlan_ingress(untagged, access);
    suite.expect_true(accepted(access_result) != nullptr,
                      "accept untagged frame on access port");
    if (accepted(access_result) != nullptr) {
        suite.expect_equal(accepted(access_result)->vlan(), vlan(10U),
                           "assign access VLAN to untagged frame");
    }

    const auto tagged_access_result =
        switching::classify_vlan_ingress(tagged_ten, access);
    suite.expect_true(dropped(tagged_access_result) != nullptr,
                      "drop tagged frame on access port");
    if (dropped(tagged_access_result) != nullptr) {
        suite.expect_equal(
            dropped(tagged_access_result)->reason(),
            switching::VlanIngressDropReason::tagged_frame_on_access_port,
            "report access-port tag violation");
    }

    const auto trunk = switching::VlanPortConfig::trunk(
        {vlan(10U), vlan(20U)}, vlan(10U)).value();
    const auto native_result =
        switching::classify_vlan_ingress(untagged, trunk);
    suite.expect_true(accepted(native_result) != nullptr,
                      "accept untagged frame on native-VLAN trunk");
    if (accepted(native_result) != nullptr) {
        suite.expect_equal(accepted(native_result)->vlan(), vlan(10U),
                           "assign native VLAN to untagged trunk frame");
    }

    const auto tagged_result =
        switching::classify_vlan_ingress(tagged_twenty, trunk);
    suite.expect_true(accepted(tagged_result) != nullptr,
                      "accept allowed tagged trunk frame");
    if (accepted(tagged_result) != nullptr) {
        suite.expect_equal(accepted(tagged_result)->vlan(), vlan(20U),
                           "preserve tagged trunk VLAN");
    }

    const auto disallowed_result =
        switching::classify_vlan_ingress(frame(vlan(30U)), trunk);
    suite.expect_true(dropped(disallowed_result) != nullptr,
                      "drop disallowed tagged trunk frame");
    if (dropped(disallowed_result) != nullptr) {
        suite.expect_equal(
            dropped(disallowed_result)->reason(),
            switching::VlanIngressDropReason::vlan_not_allowed,
            "report disallowed trunk VLAN");
    }

    const auto tagged_only =
        switching::VlanPortConfig::trunk({vlan(20U)}).value();
    const auto no_native_result =
        switching::classify_vlan_ingress(untagged, tagged_only);
    suite.expect_true(dropped(no_native_result) != nullptr,
                      "drop untagged frame on trunk without native VLAN");
    if (dropped(no_native_result) != nullptr) {
        suite.expect_equal(
            dropped(no_native_result)->reason(),
            switching::VlanIngressDropReason::untagged_frame_without_native_vlan,
            "report missing trunk native VLAN");
    }

    const auto tagged_native_result =
        switching::classify_vlan_ingress(tagged_ten, trunk);
    suite.expect_true(accepted(tagged_native_result) != nullptr,
                      "accept explicitly tagged native VLAN");
    suite.expect_equal(tagged_ten.serialize(), original_tagged_bytes,
                       "VLAN ingress classification preserves frame");
}

}  // namespace silicon_switch::test
