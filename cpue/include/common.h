#pragma once

#include <type_traits>
#include <cstring>
#include <cstdio>
#include <initializer_list>
#include <algorithm>
#include <cstdint>
#include <cassert>

#include "logging.h"


#define CPUE_ASSERT(cond, msg) assert((cond) && (msg))


namespace CPUE {



[[noreturn]] constexpr void fail(char const* msg = NULL) {
    if (msg != NULL)
        printf("%s\n", msg);
    exit(1);
}

constexpr void TODO_NOFAIL(char const* msg = NULL) {
    if (msg != NULL)
        printf("TODO: %s\n", msg);
}

[[noreturn]] constexpr void TODO(char const* msg = NULL) {
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

constexpr u64 bitmask(u8 high) {
    return (high >= 64) ? ~0ULL : (static_cast<u64>(1) << high) - 1;
}
constexpr u64 bytemask(u8 high) {
    return bitmask(high * 8);
}

// TODO: maybe rename to bit_extract or extract_bits
template<typename T>
constexpr T bits(T val, u8 high, u8 low) {
    return (val & bitmask(high + 1)) >> low;
}


template<typename T>
requires std::is_integral_v<T>
constexpr u8 sign_bit(T value) {
    return value >> (sizeof(T) * 8 - 1);
}

template<class... T>
constexpr bool always_false = false;

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
concept is_integral = std::is_integral_v<T>;
template<typename T>
concept integral = is_integral<T>;

template<typename T>
concept is_unsigned_integral = is_integral<T> && std::is_unsigned_v<T>;
template<typename T>
concept unsigned_integral = is_unsigned_integral<T>;

template<typename T>
concept is_signed_integral = is_integral<T> && std::is_signed_v<T>;
template<typename T>
concept signed_integral = is_signed_integral<T>;


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


class ByteWidth {
public:
    enum Width : u8 { WIDTH_BYTE = 1, WIDTH_WORD = 2, WIDTH_DWORD = 4, WIDTH_QWORD = 8, WIDTH_DQWORD = 16 };

    ByteWidth() = default;
    constexpr ByteWidth(Width width) : m_width(width) {}
    constexpr ByteWidth(u8 width) : m_width(static_cast<Width>(width)) {}

    // Allow switch and comparisons.
    constexpr operator Width() const { return m_width; }

    // Prevent usage in if statement
    explicit operator bool() const = delete;

    constexpr u64 bitmask() const { return bytemask(m_width); }

    template<typename T, typename Func>
    constexpr T do_with_concrete_type(Func& f) const {
        switch (m_width) {
            case WIDTH_BYTE: return f.template operator()<u8>();
            case WIDTH_WORD: return f.template operator()<u16>();
            case WIDTH_DWORD: return f.template operator()<u32>();
            case WIDTH_QWORD: return f.template operator()<u64>();
            default: fail("Can't convert to concrete type.");
        }
    }

private:
    Width m_width;
};


template<typename T>
requires std::is_integral_v<T>
constexpr ByteWidth get_byte_width() {
    switch (sizeof(T)) {
        case 1: return ByteWidth::WIDTH_BYTE;
        case 2: return ByteWidth::WIDTH_WORD;
        case 4: return ByteWidth::WIDTH_DWORD;
        case 8: return ByteWidth::WIDTH_QWORD;
        case 16: return ByteWidth::WIDTH_DQWORD;
        default: fail("Unsupported byte width");
    }
}




}