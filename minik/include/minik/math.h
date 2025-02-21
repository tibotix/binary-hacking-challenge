#ifndef MATH_H
#define MATH_H

#include "stl.h"

// rand

u64 log2(u64 val);
u64 log10(u64 val);
inline u64 log(u64 val, u64 base);
inline u64 align_to_next_pow2(u64 val);

#endif // MATH_H
