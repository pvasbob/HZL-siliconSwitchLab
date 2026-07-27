#include "silicon_switch/network/ipv4_prefix.hpp"

#include <charconv>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace silicon_switch::network {

std::optional<Ipv4Prefix> Ipv4Prefix::parse(
    const std::string_view text) noexcept {
    const std::size_t separator_position = text.find('/');
    if (separator_position == std::string_view::npos ||
        text.find('/', separator_position + 1U) != std::string_view::npos) {
        return std::nullopt;
    }

    const std::string_view address_text = text.substr(0, separator_position);
    const std::string_view length_text = text.substr(separator_position + 1U);
    const auto address = Ipv4Address::parse(address_text);
    if (!address.has_value() || length_text.empty() ||
        (length_text.size() > 1U && length_text.front() == '0')) {
        return std::nullopt;
    }

    unsigned int parsed_length = 0;
    const char* const first = length_text.data();
    const char* const last = first + length_text.size();
    const auto conversion = std::from_chars(first, last, parsed_length);

    if (conversion.ec != std::errc{} || conversion.ptr != last ||
        parsed_length > maximum_length) {
        return std::nullopt;
    }

    return create(
        *address,
        static_cast<Length>(parsed_length));
}

std::string Ipv4Prefix::to_string() const {
    std::string result = network_address_.to_string();
    result.push_back('/');
    result += std::to_string(length_);
    return result;
}

}  // namespace silicon_switch::network
