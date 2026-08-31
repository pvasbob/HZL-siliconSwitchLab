#pragma once

#include "silicon_switch/visualization/topology.hpp"

#include <vector>

namespace silicon_switch::visualization {

struct RenderVertex {
    float x;
    float y;
    float red;
    float green;
    float blue;
};

struct TopologyFrame {
    std::vector<RenderVertex> links;
    std::vector<RenderVertex> nodes;
};

class TopologyRenderer {
public:
    [[nodiscard]] TopologyFrame build_frame(const TopologyScene& scene) const;
};

}  // namespace silicon_switch::visualization
