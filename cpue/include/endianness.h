#pragma once

#include <bit>
#include <concepts>
#include <ranges>

#include "common.h"

namespace CPUE {

template<std::integral T>
constexpr T stub_byteswap(T value) noexcept {
    static_assert(std::has_unique_object_representations_v<T>, "T may not have padding bits");
    auto value_representation = std::bit_cast<std::array<std::byte, sizeof(T)>>(value);
    std::ranges::reverse(value_representation);
    return std::bit_cast<T>(value_representation);
}




template<unsigned_integral T>
struct LittleEndian;

template<unsigned_integral T>
struct BigEndian {
    // TODO: eventually make this explicit so we catch implicit conversions
    BigEndian(T value) : value(value) {}
    T value;

    operator long long unsigned() const { return static_cast<u64>(value); }

    LittleEndian<T> to_le() const { return {stub_byteswap(value)}; }
};


template<unsigned_integral T>
struct LittleEndian {
    LittleEndian(T value) : value(value) {}
    T value;

    operator long long unsigned() const { return static_cast<u64>(value); }

    BigEndian<T> to_be() const { return {stub_byteswap(value)}; }
};



}