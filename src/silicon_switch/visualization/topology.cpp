#include "silicon_switch/visualization/topology.hpp"

#include "silicon_switch/network/byte_order.hpp"

#include <algorithm>
#include <cstring>
#include <limits>
#include <utility>

namespace silicon_switch::visualization {
namespace {
constexpr std::uint32_t topology_magic = 0x544F5031U;

class Reader {
public:
    explicit Reader(const network::ByteView bytes) : bytes_{bytes} {}

    template <typename Integer>
    std::optional<Integer> integer() {
        const auto value = network::wire::read_big_endian<Integer>(bytes_, offset_);
        if (value.has_value()) {
            offset_ += sizeof(Integer);
        }
        return value;
    }

    std::optional<float> floating() {
        const auto bits = integer<std::uint32_t>();
        if (!bits.has_value()) {
            return std::nullopt;
        }
        float result = 0.0F;
        const auto raw = bits.value();
        std::memcpy(&result, &raw, sizeof(result));
        return result;
    }

    std::optional<std::string> string(const std::size_t size) {
        if (offset_ > bytes_.size() || size > bytes_.size() - offset_) {
            return std::nullopt;
        }
        const auto begin = bytes_.begin() + static_cast<std::ptrdiff_t>(offset_);
        offset_ += size;
        return std::string(begin, begin + static_cast<std::ptrdiff_t>(size));
    }

    [[nodiscard]] bool finished() const noexcept { return offset_ == bytes_.size(); }

private:
    network::ByteView bytes_;
    std::size_t offset_{0U};
};

template <typename Integer>
void append_integer(std::vector<std::uint8_t>& bytes, const Integer value) {
    const auto offset = bytes.size();
    bytes.resize(offset + sizeof(Integer));
    static_cast<void>(network::wire::write_big_endian<Integer>(
        value, network::MutableByteView{bytes}, offset));
}

void append_float(std::vector<std::uint8_t>& bytes, const float value) {
    std::uint32_t bits = 0U;
    std::memcpy(&bits, &value, sizeof(bits));
    append_integer(bytes, bits);
}
}  // namespace

std::optional<TopologyScene> TopologyScene::parse_snapshot(const network::ByteView bytes) {
    Reader reader{bytes};
    const auto magic = reader.integer<std::uint32_t>();
    const auto node_count = reader.integer<std::uint16_t>();
    const auto link_count = reader.integer<std::uint16_t>();
    if (!magic.has_value() || magic.value() != topology_magic ||
        !node_count.has_value() || !link_count.has_value()) {
        return std::nullopt;
    }

    TopologyScene scene;
    for (std::uint16_t index = 0U; index < node_count.value(); ++index) {
        const auto id = reader.integer<std::uint32_t>();
        const auto x = reader.floating();
        const auto y = reader.floating();
        const auto vlan = reader.integer<std::uint16_t>();
        const auto flags = reader.integer<std::uint8_t>();
        const auto label_size = reader.integer<std::uint8_t>();
        const auto packets = reader.integer<std::uint64_t>();
        if (!id.has_value() || !x.has_value() || !y.has_value() || !vlan.has_value() ||
            !flags.has_value() || !label_size.has_value() || !packets.has_value()) {
            return std::nullopt;
        }
        const auto label = reader.string(label_size.value());
        if (!label.has_value()) {
            return std::nullopt;
        }
        scene.upsert_node({id.value(), x.value(), y.value(), vlan.value(),
                           packets.value(), (flags.value() & 1U) != 0U, label.value()});
    }
    for (std::uint16_t index = 0U; index < link_count.value(); ++index) {
        const auto source = reader.integer<std::uint32_t>();
        const auto destination = reader.integer<std::uint32_t>();
        const auto packets = reader.integer<std::uint64_t>();
        const auto flags = reader.integer<std::uint8_t>();
        if (!source.has_value() || !destination.has_value() || !packets.has_value() ||
            !flags.has_value()) {
            return std::nullopt;
        }
        scene.upsert_link({source.value(), destination.value(), packets.value(),
                           (flags.value() & 1U) != 0U});
    }
    return reader.finished() ? std::optional<TopologyScene>{std::move(scene)} : std::nullopt;
}

std::vector<std::uint8_t> TopologyScene::serialize_snapshot() const {
    if (nodes_.size() > std::numeric_limits<std::uint16_t>::max() ||
        links_.size() > std::numeric_limits<std::uint16_t>::max()) {
        return {};
    }
    std::vector<std::uint8_t> bytes;
    append_integer(bytes, topology_magic);
    append_integer(bytes, static_cast<std::uint16_t>(nodes_.size()));
    append_integer(bytes, static_cast<std::uint16_t>(links_.size()));
    for (const auto& node : nodes_) {
        if (node.label.size() > std::numeric_limits<std::uint8_t>::max()) {
            return {};
        }
        append_integer(bytes, node.id);
        append_float(bytes, node.x);
        append_float(bytes, node.y);
        append_integer(bytes, node.vlan);
        append_integer(bytes, static_cast<std::uint8_t>(node.operational ? 1U : 0U));
        append_integer(bytes, static_cast<std::uint8_t>(node.label.size()));
        append_integer(bytes, node.packets);
        bytes.insert(bytes.end(), node.label.begin(), node.label.end());
    }
    for (const auto& link : links_) {
        append_integer(bytes, link.source);
        append_integer(bytes, link.destination);
        append_integer(bytes, link.packets);
        append_integer(bytes, static_cast<std::uint8_t>(link.operational ? 1U : 0U));
    }
    return bytes;
}

bool TopologyScene::apply_delta(const network::ByteView bytes) {
    auto delta = parse_snapshot(bytes);
    if (!delta.has_value()) {
        return false;
    }
    for (auto node : delta->nodes_) {
        upsert_node(std::move(node));
    }
    for (const auto& link : delta->links_) {
        upsert_link(link);
    }
    return true;
}

void TopologyScene::upsert_node(TopologyNode node) {
    const auto found = std::find_if(nodes_.begin(), nodes_.end(),
                                    [&node](const auto& current) { return current.id == node.id; });
    if (found == nodes_.end()) {
        nodes_.push_back(std::move(node));
    } else {
        *found = std::move(node);
    }
}

void TopologyScene::upsert_link(const TopologyLink link) {
    const auto found = std::find_if(links_.begin(), links_.end(), [&link](const auto& current) {
        return current.source == link.source && current.destination == link.destination;
    });
    if (found == links_.end()) {
        links_.push_back(link);
    } else {
        *found = link;
    }
}

}  // namespace silicon_switch::visualization
