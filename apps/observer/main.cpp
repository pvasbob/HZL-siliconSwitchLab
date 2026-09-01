#include "silicon_switch/visualization/application.hpp"
#include "silicon_switch/visualization/observer_client.hpp"
#include "silicon_switch/visualization/topology_renderer.hpp"

#include <GLFW/glfw3.h>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <variant>

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

namespace {
visualization::TopologyScene demo_scene() {
    visualization::TopologyScene scene;
    scene.upsert_node({1U, -0.65F, 0.15F, 10U, 850U, true, "switch"});
    scene.upsert_node({2U, 0.0F, 0.55F, 20U, 420U, true, "observer-a"});
    scene.upsert_node({3U, 0.65F, 0.15F, 20U, 100U, true, "observer-b"});
    scene.upsert_node({4U, 0.0F, -0.55F, 30U, 0U, false, "observer-c"});
    scene.upsert_link({1U, 2U, 700U, true});
    scene.upsert_link({1U, 3U, 250U, true});
    scene.upsert_link({1U, 4U, 0U, false});
    return scene;
}

std::optional<std::uint16_t> parse_port(const std::string& value) {
    try {
        std::size_t parsed = 0U;
        const auto port = std::stoul(value, &parsed);
        if (parsed != value.size() || port == 0U || port > 65'535U) {
            return std::nullopt;
        }
        return static_cast<std::uint16_t>(port);
    } catch (const std::exception&) {
        return std::nullopt;
    }
}
}  // namespace

int main(int argc, char** argv) {
    try {
        if (argc == 1) {
            const auto scene = demo_scene();
            GlfwWindow window;
            visualization::VisualizationApplication application{window};
            static_cast<void>(application.run(scene));
            return EXIT_SUCCESS;
        }

        std::string address{"0.0.0.0"};
        std::string name{"observer"};
        std::optional<std::uint16_t> port;
        for (int index = 1; index < argc; ++index) {
            const std::string argument{argv[index]};
            if ((argument == "--listen" || argument == "--port" || argument == "--name") &&
                index + 1 >= argc) {
                port.reset();
                break;
            }
            if (argument == "--listen") {
                address = argv[++index];
            } else if (argument == "--port") {
                port = parse_port(argv[++index]);
            } else if (argument == "--name") {
                name = argv[++index];
            } else {
                port.reset();
                break;
            }
        }
        if (!port.has_value()) {
            std::cerr << "usage: silicon_switch_observer --port PORT "
                         "[--listen IPv4] [--name NAME]\n";
            return EXIT_FAILURE;
        }
        auto bound = visualization::ObserverClient::bind(address, port.value());
        if (!std::holds_alternative<visualization::ObserverClient>(bound)) {
            std::cerr << "failed to bind observer state port\n";
            return EXIT_FAILURE;
        }
        auto client = std::get<visualization::ObserverClient>(std::move(bound));
        static_cast<void>(client.set_receive_timeout(std::chrono::milliseconds{200}));

        visualization::TopologyScene shared_scene;
        std::mutex scene_mutex;
        std::atomic<bool> receiving{true};
        std::atomic<std::size_t> snapshots{0U};
        std::atomic<std::size_t> deltas{0U};
        std::atomic<std::size_t> resyncs{0U};
        std::atomic<std::uint64_t> revision{0U};
        GlfwWindow window;
        std::thread receiver{
            [client = std::move(client), &shared_scene, &scene_mutex, &receiving,
             &snapshots, &deltas, &resyncs, &revision]() mutable {
                while (receiving.load()) {
                    const auto result = client.receive_next();
                    if (result == visualization::ObserverResult::applied_snapshot ||
                        result == visualization::ObserverResult::applied_delta) {
                        {
                            std::lock_guard<std::mutex> lock{scene_mutex};
                            shared_scene = client.scene();
                        }
                        revision.store(client.synchronizer().revision());
                        if (result == visualization::ObserverResult::applied_snapshot) {
                            ++snapshots;
                        } else {
                            ++deltas;
                        }
                    } else if (result == visualization::ObserverResult::resync_required) {
                        ++resyncs;
                    }
                }
            }};

        while (!window.should_close()) {
            visualization::TopologyScene frame_scene;
            {
                std::lock_guard<std::mutex> lock{scene_mutex};
                frame_scene = shared_scene;
            }
            window.poll_events();
            window.present(frame_scene);
        }
        receiving.store(false);
        receiver.join();
        std::cout << "observer=" << name << " snapshots=" << snapshots.load()
                  << " deltas=" << deltas.load() << " resyncs=" << resyncs.load()
                  << " revision=" << revision.load() << '\n';
        return snapshots.load() > 0U ? EXIT_SUCCESS : EXIT_FAILURE;
    } catch (const std::exception& error) {
        std::cerr << "observer error: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
