#pragma once

#include <cstdint>

namespace CPUE {

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

#ifdef __SIZEOF_INT128__
typedef __uint128_t u128;
typedef __int128_t i128;
#else
static_assert(false);
// TODO: provide custom fallback u128 class implementation
#endif



constexpr u128 make_u128(uint64_t high, uint64_t low) {
    return (static_cast<u128>(high) << 64) | low;
}

}

namespace std {
template<>
struct is_unsigned<CPUE::u128> : std::true_type {};
template<>
struct is_signed<CPUE::u128> : std::false_type {};
template<>
struct is_unsigned<CPUE::i128> : std::false_type {};
template<>
struct is_signed<CPUE::i128> : std::true_type {};
}