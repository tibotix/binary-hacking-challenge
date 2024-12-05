#pragma once

#include <type_traits>
#include <cstring>
#include <cstdio>
#include <initializer_list>
#include <algorithm>
#include <cstdint>
#include <cassert>


#define CPUE_ASSERT(cond, msg) assert((cond) && (msg))


namespace CPUE {



[[noreturn]] inline void fail(char const* msg = NULL) {
    if (msg != NULL)
        printf("%s\n", msg);
    exit(1);
}

inline void TODO_NOFAIL(char const* msg = NULL) {
    if (msg != NULL)
        printf("TODO: %s\n", msg);
}

[[noreturn]] inline void TODO(char const* msg = NULL) {
    fail(msg);
}




typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

constexpr unsigned long long operator""_kb(unsigned long long bytes) {
    return bytes * 1024;
}
constexpr unsigned long long operator""_mb(unsigned long long bytes) {
    return bytes * 1024_kb;
}
constexpr unsigned long long operator""_gb(unsigned long long bytes) {
    return bytes * 1024_mb;
}


typedef u16 PCID;

constexpr u64 bitmask64(u8 high, u8 low) {
    return ((1 << (high + 1)) - 1);
}

template<typename T>
constexpr T bits(T val, u8 high, u8 low) {
    return (val & bitmask64(high, low)) >> low;
}


template<class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;


template<typename T, typename I>
concept SameSize = (sizeof(T) == sizeof(I));

template<typename T, typename I>
requires SameSize<T, I>
static constexpr T instance(I initializer) {
    typename std::aligned_storage<sizeof(T), alignof(T)>::type storage;
    std::memcpy(&storage, &initializer, sizeof(storage));
    return *reinterpret_cast<T*>(&storage);
}
template<typename R, typename T>
requires std::is_integral_v<R> && SameSize<R, T>
static constexpr R raw_bytes(T* instance) {
    return *((R*)instance);
}


template<typename T>
concept HasReservedBits = requires(T t) {
    { t.reserved_bits_ored() } -> std::convertible_to<u64>;
};



template<typename T>
bool one_of(T value, std::initializer_list<T> validValues) {
    return std::find(validValues.begin(), validValues.end(), value) != validValues.end();
}

enum MemoryOp {
    OP_READ,
    OP_WRITE,
};

enum ByteWidth : u8 {
    WIDTH_BYTE = 1,
    WIDTH_WORD = 2,
    WIDTH_DWORD = 4,
    WIDTH_QWORD = 8,
    WIDTH_DQWORD = 16,
};

template<typename T>
requires std::is_integral_v<T>
constexpr ByteWidth get_byte_width() {
    switch (sizeof(T)) {
        case 1: return WIDTH_BYTE;
        case 2: return WIDTH_WORD;
        case 4: return WIDTH_DWORD;
        case 8: return WIDTH_QWORD;
        case 16: return WIDTH_DQWORD;
        default: fail("Unsupported byte width");
    }
}




}