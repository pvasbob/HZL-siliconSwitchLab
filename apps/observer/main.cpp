#include "silicon_switch/visualization/application.hpp"
#include "silicon_switch/visualization/topology_renderer.hpp"

#include <GLFW/glfw3.h>

#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace visualization = silicon_switch::visualization;

class GlfwWindow final : public visualization::ApplicationWindow {
public:
    GlfwWindow() {
        if (glfwInit() == GLFW_FALSE) {
            throw std::runtime_error{"GLFW initialization failed"};
        }
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
        window_ = glfwCreateWindow(960, 640, "Silicon Switch Observer", nullptr, nullptr);
        if (window_ == nullptr) {
            glfwTerminate();
            throw std::runtime_error{"OpenGL window creation failed"};
        }
        glfwMakeContextCurrent(window_);
        glfwSwapInterval(1);
        glfwSetWindowUserPointer(window_, this);
        glfwSetKeyCallback(window_, [](GLFWwindow* window, int key, int, int action, int) {
            auto* self = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(window));
            if (action != GLFW_PRESS && action != GLFW_REPEAT) {
                return;
            }
            if (key == GLFW_KEY_ESCAPE) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            } else if (key == GLFW_KEY_LEFT) {
                self->offset_x_ -= 0.05F;
            } else if (key == GLFW_KEY_RIGHT) {
                self->offset_x_ += 0.05F;
            } else if (key == GLFW_KEY_UP) {
                self->offset_y_ += 0.05F;
            } else if (key == GLFW_KEY_DOWN) {
                self->offset_y_ -= 0.05F;
            }
        });
    }

    ~GlfwWindow() override {
        glfwDestroyWindow(window_);
        glfwTerminate();
    }

    [[nodiscard]] bool should_close() const override {
        return glfwWindowShouldClose(window_) != GLFW_FALSE;
    }

    void poll_events() override { glfwPollEvents(); }

    void present(const visualization::TopologyScene& scene) override {
        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(window_, &width, &height);
        glViewport(0, 0, width, height);
        glClearColor(0.025F, 0.035F, 0.065F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT);
        glLoadIdentity();
        glTranslatef(offset_x_, offset_y_, 0.0F);

        const auto frame = renderer_.build_frame(scene);
        glLineWidth(3.0F);
        glBegin(GL_LINES);
        for (const auto& vertex : frame.links) {
            glColor3f(vertex.red, vertex.green, vertex.blue);
            glVertex2f(vertex.x, vertex.y);
        }
        glEnd();
        glPointSize(18.0F);
        glBegin(GL_POINTS);
        for (const auto& vertex : frame.nodes) {
            glColor3f(vertex.red, vertex.green, vertex.blue);
            glVertex2f(vertex.x, vertex.y);
        }
        glEnd();
        glfwSwapBuffers(window_);
    }

private:
    GLFWwindow* window_{nullptr};
    visualization::TopologyRenderer renderer_;
    float offset_x_{0.0F};
    float offset_y_{0.0F};
};

int main() {
    try {
        visualization::TopologyScene scene;
        scene.upsert_node({1U, -0.65F, 0.15F, 10U, 850U, true, "switch"});
        scene.upsert_node({2U, 0.0F, 0.55F, 20U, 420U, true, "observer-a"});
        scene.upsert_node({3U, 0.65F, 0.15F, 20U, 100U, true, "observer-b"});
        scene.upsert_node({4U, 0.0F, -0.55F, 30U, 0U, false, "observer-c"});
        scene.upsert_link({1U, 2U, 700U, true});
        scene.upsert_link({1U, 3U, 250U, true});
        scene.upsert_link({1U, 4U, 0U, false});

        GlfwWindow window;
        visualization::VisualizationApplication application{window};
        static_cast<void>(application.run(scene));
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "observer error: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
