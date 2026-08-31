#include "visualization/visualization_test.hpp"

#include "silicon_switch/transport/state_update_protocol.hpp"
#include "silicon_switch/visualization/application.hpp"
#include "silicon_switch/visualization/observer_client.hpp"
#include "silicon_switch/visualization/topology_renderer.hpp"
#include "test_support.hpp"

#include <cstddef>
#include <variant>

namespace silicon_switch::test {
namespace {
class HeadlessWindow final : public visualization::ApplicationWindow {
public:
    [[nodiscard]] bool should_close() const override { return false; }
    void poll_events() override { ++polls; }
    void present(const visualization::TopologyScene& scene) override {
        ++presentations;
        last_node_count = scene.nodes().size();
    }

    std::size_t polls{0U};
    std::size_t presentations{0U};
    std::size_t last_node_count{0U};
};
}  // namespace

void run_visualization_tests(TestSuite& suite) {
    visualization::TopologyScene scene;
    scene.upsert_node({1U, -0.5F, 0.0F, 10U, 100U, true, "switch"});
    scene.upsert_node({2U, 0.5F, 0.0F, 20U, 25U, true, "observer"});
    scene.upsert_link({1U, 2U, 80U, true});
    suite.expect_equal(scene.nodes().size(), std::size_t{2U}, "store topology nodes");
    suite.expect_equal(scene.links().size(), std::size_t{1U}, "store topology links");

    const auto encoded = scene.serialize_snapshot();
    const auto decoded = visualization::TopologyScene::parse_snapshot(encoded);
    suite.expect_true(decoded.has_value(), "round-trip topology snapshot");
    suite.expect_equal(decoded->nodes().at(0U).label, std::string{"switch"},
                       "preserve topology node label");
    suite.expect_equal(decoded->links().at(0U).packets, std::uint64_t{80U},
                       "preserve topology link counters");
    auto malformed = encoded;
    malformed.pop_back();
    suite.expect_false(visualization::TopologyScene::parse_snapshot(malformed).has_value(),
                       "reject truncated topology snapshot");

    visualization::TopologyRenderer renderer;
    const auto frame = renderer.build_frame(scene);
    suite.expect_equal(frame.links.size(), std::size_t{2U},
                       "render each topology link as two vertices");
    suite.expect_equal(frame.nodes.size(), std::size_t{2U},
                       "render each topology node");

    HeadlessWindow window;
    visualization::VisualizationApplication application{window};
    suite.expect_equal(application.run(scene, 3U), std::size_t{3U},
                       "run bounded headless render loop");
    suite.expect_equal(window.polls, std::size_t{3U}, "poll input for each frame");
    suite.expect_equal(window.presentations, std::size_t{3U}, "present each frame");
    suite.expect_equal(window.last_node_count, std::size_t{2U},
                       "provide topology state to renderer");

    auto observer_result = visualization::ObserverClient::bind("127.0.0.1", 0U);
    suite.expect_true(std::holds_alternative<visualization::ObserverClient>(observer_result),
                      "bind observer UDP client");
    if (!std::holds_alternative<visualization::ObserverClient>(observer_result)) {
        return;
    }
    auto observer = std::get<visualization::ObserverClient>(std::move(observer_result));
    const auto snapshot = transport::StateUpdate::create(
        transport::StateUpdateType::snapshot, 77U, 1U, 1'000U, 0U, 1U, encoded);
    suite.expect_equal(observer.consume_datagram(snapshot->serialize()),
                       visualization::ObserverResult::applied_snapshot,
                       "observer applies authoritative topology snapshot");
    suite.expect_equal(observer.scene().nodes().size(), std::size_t{2U},
                       "observer exposes synchronized topology");

    visualization::TopologyScene delta_scene;
    delta_scene.upsert_node({2U, 0.6F, 0.1F, 20U, 50U, true, "observer"});
    const auto delta = transport::StateUpdate::create(
        transport::StateUpdateType::delta, 77U, 2U, 2'000U, 1U, 2U,
        delta_scene.serialize_snapshot());
    suite.expect_equal(observer.consume_datagram(delta->serialize()),
                       visualization::ObserverResult::applied_delta,
                       "observer applies topology delta");
    suite.expect_equal(observer.scene().nodes().at(1U).packets, std::uint64_t{50U},
                       "observer updates topology counters");
    const auto gap = transport::StateUpdate::create(
        transport::StateUpdateType::delta, 77U, 4U, 4'000U, 2U, 3U,
        delta_scene.serialize_snapshot());
    suite.expect_equal(observer.consume_datagram(gap->serialize()),
                       visualization::ObserverResult::resync_required,
                       "observer requests snapshot after packet loss");
    suite.expect_equal(observer.consume_datagram(std::vector<std::uint8_t>{1U}),
                       visualization::ObserverResult::malformed_update,
                       "observer rejects malformed datagram");
}

}  // namespace silicon_switch::test
