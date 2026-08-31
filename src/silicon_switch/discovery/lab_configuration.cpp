#include "silicon_switch/discovery/lab_configuration.hpp"

#include <algorithm>
#include <iterator>
#include <utility>

namespace silicon_switch::discovery {

ConfigurationResult LabConfiguration::add_or_update(DiscoveredMachine machine) {
    if (machine.node_id == 0U || machine.name.empty() || machine.address.empty() ||
        machine.role == MachineRole::unspecified || machine.state_port == 0U ||
        (machine.role == MachineRole::leader && machine.control_port == 0U)) {
        return ConfigurationResult::rejected_invalid;
    }
    const auto same_node = std::find_if(machines_.begin(), machines_.end(),
        [&machine](const auto& current) { return current.node_id == machine.node_id; });
    const auto conflict = std::find_if(machines_.begin(), machines_.end(),
        [&machine](const auto& current) {
            return current.node_id != machine.node_id &&
                   (current.name == machine.name || current.address == machine.address);
        });
    if (conflict != machines_.end()) {
        return ConfigurationResult::rejected_conflict;
    }
    if (same_node == machines_.end()) {
        machines_.push_back(std::move(machine));
        return ConfigurationResult::added;
    }
    *same_node = std::move(machine);
    return ConfigurationResult::updated;
}

bool LabConfiguration::complete() const noexcept {
    return machines_.size() == 4U &&
           std::count_if(machines_.begin(), machines_.end(), [](const auto& machine) {
               return machine.role == MachineRole::leader;
           }) == 1 &&
           std::count_if(machines_.begin(), machines_.end(), [](const auto& machine) {
               return machine.role == MachineRole::observer;
           }) == 3;
}

const DiscoveredMachine* LabConfiguration::leader() const noexcept {
    const auto found = std::find_if(machines_.begin(), machines_.end(), [](const auto& machine) {
        return machine.role == MachineRole::leader;
    });
    return found == machines_.end() ? nullptr : &*found;
}

std::vector<DiscoveredMachine> LabConfiguration::observers() const {
    std::vector<DiscoveredMachine> result;
    std::copy_if(machines_.begin(), machines_.end(), std::back_inserter(result),
                 [](const auto& machine) { return machine.role == MachineRole::observer; });
    return result;
}

}  // namespace silicon_switch::discovery
