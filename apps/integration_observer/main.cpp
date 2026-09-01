#include "silicon_switch/visualization/observer_client.hpp"

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <variant>

namespace visualization = silicon_switch::visualization;
namespace {
struct Options {
    std::string address{"0.0.0.0"};
    std::string name{"observer"};
    std::uint16_t port{0U};
    std::size_t updates{60U};
    std::uint64_t timeout_ms{10'000U};
};

std::optional<std::uint64_t> number(const std::string& value) {
    try {
        std::size_t parsed = 0U;
        const auto result = std::stoull(value, &parsed);
        return parsed == value.size() ? std::optional<std::uint64_t>{result} : std::nullopt;
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

std::optional<Options> parse_options(int argc, char** argv) {
    Options options;
    for (int index = 1; index < argc; ++index) {
        const std::string argument{argv[index]};
        if ((argument == "--listen" || argument == "--name" || argument == "--port" ||
             argument == "--updates" || argument == "--timeout-ms") && index + 1 >= argc) {
            return std::nullopt;
        }
        if (argument == "--listen") {
            options.address = argv[++index];
        } else if (argument == "--name") {
            options.name = argv[++index];
        } else if (argument == "--port") {
            const auto value = number(argv[++index]);
            if (!value.has_value() || value.value() == 0U || value.value() > 65'535U) {
                return std::nullopt;
            }
            options.port = static_cast<std::uint16_t>(value.value());
        } else if (argument == "--updates") {
            const auto value = number(argv[++index]);
            if (!value.has_value() || value.value() == 0U) {
                return std::nullopt;
            }
            options.updates = static_cast<std::size_t>(value.value());
        } else if (argument == "--timeout-ms") {
            const auto value = number(argv[++index]);
            if (!value.has_value() || value.value() == 0U) {
                return std::nullopt;
            }
            options.timeout_ms = value.value();
        } else {
            return std::nullopt;
        }
    }
    return options.port == 0U ? std::nullopt : std::optional<Options>{options};
}

std::string escape_json(const std::string& value) {
    std::string result;
    for (const char character : value) {
        if (character == '"' || character == '\\') {
            result.push_back('\\');
        }
        result.push_back(character);
    }
    return result;
}
}  // namespace

int main(int argc, char** argv) {
    const auto options = parse_options(argc, argv);
    if (!options.has_value()) {
        std::cerr << "usage: silicon_switch_observer_node --port PORT "
                     "[--listen IPv4] [--name NAME] [--updates COUNT] "
                     "[--timeout-ms MILLISECONDS]\n";
        return EXIT_FAILURE;
    }
    auto bound = visualization::ObserverClient::bind(options->address, options->port);
    if (!std::holds_alternative<visualization::ObserverClient>(bound)) {
        std::cerr << "failed to bind observer state port\n";
        return EXIT_FAILURE;
    }
    auto observer = std::get<visualization::ObserverClient>(std::move(bound));
    static_cast<void>(observer.set_receive_timeout(std::chrono::milliseconds{250}));

    std::size_t snapshots = 0U;
    std::size_t deltas = 0U;
    std::size_t resyncs = 0U;
    std::size_t malformed = 0U;
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds{options->timeout_ms};
    while (snapshots + deltas < options->updates &&
           std::chrono::steady_clock::now() < deadline) {
        switch (observer.receive_next()) {
            case visualization::ObserverResult::applied_snapshot:
                ++snapshots;
                break;
            case visualization::ObserverResult::applied_delta:
                ++deltas;
                break;
            case visualization::ObserverResult::resync_required:
                ++resyncs;
                break;
            case visualization::ObserverResult::malformed_update:
            case visualization::ObserverResult::malformed_scene:
                ++malformed;
                break;
            case visualization::ObserverResult::ignored:
            case visualization::ObserverResult::network_error:
                break;
        }
    }
    const bool success = snapshots > 0U && snapshots + deltas >= options->updates &&
                         observer.synchronizer().synchronized();
    std::cout << "{\"observer\":\"" << escape_json(options->name)
              << "\",\"synchronized\":" << (success ? "true" : "false")
              << ",\"snapshots\":" << snapshots
              << ",\"deltas\":" << deltas
              << ",\"resyncs\":" << resyncs
              << ",\"malformed\":" << malformed
              << ",\"revision\":" << observer.synchronizer().revision()
              << ",\"nodes\":" << observer.scene().nodes().size() << "}\n";
    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
