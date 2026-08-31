#pragma once

#include "silicon_switch/visualization/topology.hpp"

#include <cstddef>

namespace silicon_switch::visualization {

class ApplicationWindow {
public:
    virtual ~ApplicationWindow() = default;
    [[nodiscard]] virtual bool should_close() const = 0;
    virtual void poll_events() = 0;
    virtual void present(const TopologyScene& scene) = 0;
};

class VisualizationApplication {
public:
    explicit VisualizationApplication(ApplicationWindow& window) : window_{window} {}
    [[nodiscard]] std::size_t run(const TopologyScene& scene,
                                  std::size_t maximum_frames = 0U);

private:
    ApplicationWindow& window_;
};

}  // namespace silicon_switch::visualization
