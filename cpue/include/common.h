#pragma once


#include <cstdio>
#include <stdlib.h>
#include <cstdint>
#include <cassert>


#define CPUE_ASSERT(cond, msg) assert((cond) && (msg))


namespace CPUE {


typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;



constexpr u64 bitmask64(u8 high, u8 low) {
    return ((1 << high + 1) - 1) & ~((1 << low + 1) - 1);
}

template<typename T>
constexpr T bits(T val, u8 high, u8 low) {
    return val & bitmask64(high, low);
}

enum ByteWidth : u8 {
    BYTE = 1,
    WORD = 2,
    DWORD = 4,
    QWORD = 8,
    DQWORD = 16,
};


constexpr unsigned long long operator""_kb(unsigned long long bytes) {
    return bytes * 1024;
}
constexpr unsigned long long operator""_mb(unsigned long long bytes) {
    return bytes * 1024_kb;
}
constexpr unsigned long long operator""_gb(unsigned long long bytes) {
    return bytes * 1024_mb;
}


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


}