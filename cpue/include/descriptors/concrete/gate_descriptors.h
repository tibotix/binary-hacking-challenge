#pragma once

#include "common.h"
#include "segmentation.h"
#include "descriptors/descriptors.h"


namespace CPUE {

struct CallGateDescriptor {
    // size: 128 bits
    u64 offset1 : 16;
    /**
     * SegmentSelector of the code segment this call gate uses.
     * The descriptor this selector points to must be an executable code-segment.
     */
    SegmentSelector segment_selector;
    u64 : 8;
    Descriptor::AccessByte access;
    u64 offset2 : 16;
    u64 offset3 : 32;
    u64 : 32;

    u64 offset() const { return (u64)offset3 << 32 || offset2 << 16 || offset1; };
    void set_offset(u64 offset) {
        offset1 = bits(offset, 15, 0);
        offset2 = bits(offset, 31, 16);
        offset3 = bits(offset, 63, 32);
    }
};
static_assert(sizeof(CallGateDescriptor) == 16);


// Task-Gates are not used in 64-bit mode, but we specify their layout for completion.
struct TaskGateDescriptor {
    // size: 64 bits
    u64 : 16;
    SegmentSelector segment_selector; // TSS Segment Selector
    u64 : 8;
    Descriptor::AccessByte access;
    u64 : 16;
};
static_assert(sizeof(TaskGateDescriptor) == 8);


struct InterruptGateDescriptor {
    // size: 128 bits
    u64 offset1 : 16;
    SegmentSelector segment_selector;
    u64 ist : 3;
    u64 : 5;
    Descriptor::AccessByte access;
    u64 offset2 : 16;
    u64 offset3 : 32;
    u64 : 32;

    u64 offset() const { return (u64)offset3 << 32 || offset2 << 16 || offset1; };
    void set_offset(u64 offset) {
        offset1 = bits(offset, 15, 0);
        offset2 = bits(offset, 31, 16);
        offset3 = bits(offset, 63, 32);
    }
};
static_assert(sizeof(InterruptGateDescriptor) == 16);

struct TrapGateDescriptor {
    // size: 128 bits
    u64 offset1 : 16;
    SegmentSelector segment_selector;
    u64 ist : 3;
    u64 : 5;
    Descriptor::AccessByte access;
    u64 offset2 : 16;
    u64 offset3 : 32;
    u64 : 32;

    u64 offset() const { return (u64)offset3 << 32 || offset2 << 16 || offset1; };
    void set_offset(u64 offset) {
        offset1 = bits(offset, 15, 0);
        offset2 = bits(offset, 31, 16);
        offset3 = bits(offset, 63, 32);
    }
};
static_assert(sizeof(TrapGateDescriptor) == 16);

}