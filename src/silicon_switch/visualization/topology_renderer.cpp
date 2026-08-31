#include "silicon_switch/visualization/topology_renderer.hpp"

#include <algorithm>

namespace silicon_switch::visualization {
namespace {
RenderVertex vertex(const TopologyNode& node, const bool operational,
                    const std::uint64_t packets) {
    const float activity = std::min(1.0F, static_cast<float>(packets) / 1000.0F);
    if (!operational) {
        return {node.x, node.y, 0.85F, 0.15F, 0.15F};
    }
    const float vlan_color = static_cast<float>(node.vlan % 7U) / 7.0F;
    return {node.x, node.y, 0.15F + activity * 0.55F,
            0.45F + vlan_color * 0.4F, 0.95F - activity * 0.35F};
}
}  // namespace

TopologyFrame TopologyRenderer::build_frame(const TopologyScene& scene) const {
    TopologyFrame frame;
    for (const auto& link : scene.links()) {
        const auto source = std::find_if(scene.nodes().begin(), scene.nodes().end(),
            [&link](const auto& node) { return node.id == link.source; });
        const auto destination = std::find_if(scene.nodes().begin(), scene.nodes().end(),
            [&link](const auto& node) { return node.id == link.destination; });
        if (source == scene.nodes().end() || destination == scene.nodes().end()) {
            continue;
        }
        frame.links.push_back(vertex(*source, link.operational, link.packets));
        frame.links.push_back(vertex(*destination, link.operational, link.packets));
    }
    for (const auto& node : scene.nodes()) {
        frame.nodes.push_back(vertex(node, node.operational, node.packets));
    }
    return frame;
}

}  // namespace silicon_switch::visualization
