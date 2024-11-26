#pragma once

#include "common.h"

namespace CPUE {

struct TSS {
    // size: 104 bytes
    u32 : 32;
    u32 rsp0_1;
    u32 rsp0_2;
    u32 rsp1_1;
    u32 rsp1_2;
    u32 rsp2_1;
    u32 rsp2_2;
    u32 : 32;
    u32 : 32;
    u32 ist1_1;
    u32 ist1_2;
    u32 ist2_1;
    u32 ist2_2;
    u32 ist3_1;
    u32 ist3_2;
    u32 ist4_1;
    u32 ist4_2;
    u32 ist5_1;
    u32 ist5_2;
    u32 ist6_1;
    u32 ist6_2;
    u32 ist7_1;
    u32 ist7_2;
    u32 : 32;
    u32 : 32;
    u32 : 16;
    u32 iomap_base : 16;

    u64 rsp0() const { return u64(rsp0_2) << 32 | u64(rsp0_1); }
    u64 rsp1() const { return u64(rsp1_2) << 32 | u64(rsp1_1); }
    u64 rsp2() const { return u64(rsp2_2) << 32 | u64(rsp2_1); }
    u64 ist1() const { return u64(ist1_2) << 32 | u64(ist1_1); }
    u64 ist2() const { return u64(ist2_2) << 32 | u64(ist2_1); }
    u64 ist3() const { return u64(ist3_2) << 32 | u64(ist3_1); }
    u64 ist4() const { return u64(ist4_2) << 32 | u64(ist4_1); }
    u64 ist5() const { return u64(ist5_2) << 32 | u64(ist5_1); }
    u64 ist6() const { return u64(ist6_2) << 32 | u64(ist6_1); }
    u64 ist7() const { return u64(ist7_2) << 32 | u64(ist7_1); }
};
static_assert(sizeof(TSS) == 104);


}