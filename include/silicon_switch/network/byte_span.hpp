#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

namespace silicon_switch::network {

template <typename Byte>
class BasicByteSpan {
    static_assert(
        std::is_same<typename std::remove_const<Byte>::type,
                     std::uint8_t>::value,
        "BasicByteSpan supports only byte data");

public:
    using iterator = Byte*;

    constexpr BasicByteSpan() noexcept = default;

    constexpr BasicByteSpan(Byte* data, const std::size_t size) noexcept
        : data_{data}, size_{size} {}

    template <typename Allocator,
              typename B = Byte,
              typename std::enable_if<!std::is_const<B>::value, int>::type = 0>
    BasicByteSpan(std::vector<std::uint8_t, Allocator>& bytes) noexcept
        : data_{bytes.data()}, size_{bytes.size()} {}

    template <typename Allocator>
    BasicByteSpan(const std::vector<std::uint8_t, Allocator>& bytes) noexcept
        : data_{bytes.data()}, size_{bytes.size()} {}

    template <std::size_t Size,
              typename B = Byte,
              typename std::enable_if<!std::is_const<B>::value, int>::type = 0>
    constexpr BasicByteSpan(std::array<std::uint8_t, Size>& bytes) noexcept
        : data_{bytes.data()}, size_{Size} {}

    template <std::size_t Size>
    constexpr BasicByteSpan(
        const std::array<std::uint8_t, Size>& bytes) noexcept
        : data_{bytes.data()}, size_{Size} {}

    [[nodiscard]] constexpr iterator begin() const noexcept { return data_; }
    [[nodiscard]] constexpr iterator end() const noexcept { return data_ + size_; }
    [[nodiscard]] constexpr std::size_t size() const noexcept { return size_; }
    [[nodiscard]] constexpr bool empty() const noexcept { return size_ == 0U; }
    [[nodiscard]] constexpr Byte& operator[](const std::size_t index) const noexcept {
        return data_[index];
    }

    [[nodiscard]] constexpr BasicByteSpan first(
        const std::size_t count) const noexcept {
        return BasicByteSpan{data_, count};
    }

private:
    Byte* data_{nullptr};
    std::size_t size_{0U};
};

using ByteView = BasicByteSpan<const std::uint8_t>;
using MutableByteView = BasicByteSpan<std::uint8_t>;

}  // namespace silicon_switch::network
