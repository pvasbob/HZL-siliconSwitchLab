#pragma once

#include "silicon_switch/discovery/discovery_protocol.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace silicon_switch::discovery {

struct DiscoveredMachine {
    std::uint64_t node_id;
    std::string name;
    std::string address;
    MachineRole role;
    Capability capabilities;
    std::uint16_t control_port;
    std::uint16_t state_port;
};

enum class ConfigurationResult { added, updated, rejected_conflict, rejected_invalid };

class LabConfiguration {
public:
    [[nodiscard]] ConfigurationResult add_or_update(DiscoveredMachine machine);
    [[nodiscard]] bool complete() const noexcept;
    [[nodiscard]] const DiscoveredMachine* leader() const noexcept;
    [[nodiscard]] std::vector<DiscoveredMachine> observers() const;
    [[nodiscard]] const std::vector<DiscoveredMachine>& machines() const noexcept {
        return machines_;
    }

private:
    std::vector<DiscoveredMachine> machines_;
};

}  // namespace silicon_switch::discovery
