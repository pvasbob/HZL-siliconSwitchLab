#include "silicon_switch/visualization/application.hpp"

namespace silicon_switch::visualization {

std::size_t VisualizationApplication::run(const TopologyScene& scene,
                                          const std::size_t maximum_frames) {
    std::size_t frames = 0U;
    while (!window_.should_close() && (maximum_frames == 0U || frames < maximum_frames)) {
        window_.poll_events();
        window_.present(scene);
        ++frames;
    }
    return frames;
}

}  // namespace silicon_switch::visualization
