#pragma once

#include <limits>
#include "common.h"


namespace CPUE {

template<typename R, typename... T>
requires(std::is_same_v<R, T>&&...) && (is_unsigned_integral<R>)constexpr R CPUE_checked_uadd(R const first, T const... factors) {
    // Consider calculation invalid if we are only adding one number
    static_assert(sizeof...(factors) > 0, "You have to add at least two numbers together.");

    constexpr R MAX_VAL = std::numeric_limits<R>::max();

    int i = 0;
    R res = first;

    for (const auto f : {factors...}) {
        // Fail if including this additional factor would exceed MAX_VAL
        size_t max_summand = MAX_VAL - res;
        if (f > max_summand)
            fail("Integer overflow in addition.");
        res += f;
    }
    return res;
}

template<typename R, typename... T>
requires(std::is_same_v<R, T>&&...) && (std::is_integral_v<R> && std::is_unsigned_v<R>)constexpr R CPUE_checked_umul(R const first, T const... factors) {
    // Consider calculation invalid if we are only multiplying one number
    static_assert(sizeof...(factors) > 0, "You have to multiply at least two numbers together.");

    constexpr R MAX_VAL = std::numeric_limits<R>::max();

    int i = 0;
    R res = first;

    for (const auto f : {factors...}) {
        // Fail if including this additional factor would exceed MAX_VAL
        size_t max_factor = MAX_VAL / res;
        if (f > max_factor)
            fail("Integer overflow in multiplication.");
        res *= f;
    }
    return res;
}


}