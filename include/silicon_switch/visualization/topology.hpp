#pragma once

#include "silicon_switch/network/byte_span.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace silicon_switch::visualization {

struct TopologyNode {
    std::uint32_t id;
    float x;
    float y;
    std::uint16_t vlan;
    std::uint64_t packets;
    bool operational;
    std::string label;
};

struct TopologyLink {
    std::uint32_t source;
    std::uint32_t destination;
    std::uint64_t packets;
    bool operational;
};

class TopologyScene {
public:
    [[nodiscard]] static std::optional<TopologyScene> parse_snapshot(
        network::ByteView bytes);
    [[nodiscard]] std::vector<std::uint8_t> serialize_snapshot() const;
    [[nodiscard]] bool apply_delta(network::ByteView bytes);

    void upsert_node(TopologyNode node);
    void upsert_link(TopologyLink link);
    [[nodiscard]] const std::vector<TopologyNode>& nodes() const noexcept { return nodes_; }
    [[nodiscard]] const std::vector<TopologyLink>& links() const noexcept { return links_; }

private:
    std::vector<TopologyNode> nodes_;
    std::vector<TopologyLink> links_;
};

}  // namespace silicon_switch::visualization
