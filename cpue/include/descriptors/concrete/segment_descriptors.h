#pragma once

#include "descriptors/descriptors.h"
#include "common.h"

namespace CPUE {

// Code or Data Segment Descriptor
struct ApplicationSegmentDescriptor {
    // size: 64 bits
    u64 limit1 : 16;

    /**
     * Base address fields
     * Defines the location of byte 0 of the segment within the 4-GByte linear address space. The
     * processor puts together the three base address fields to form a single 32-bit value. Segment base
     * addresses should be aligned to 16-byte boundaries. Although 16-byte alignment is not required,
     * this alignment allows programs to maximize performance by aligning code and data on 16-byte
     * boundaries
     */
    u64 base1 : 24;

    Descriptor::AccessByte access;

    u64 limit2 : 4;

    u64 avl : 1; // Available for use by system software

    /**
     * 64-bit code segment flag (IA-32e mode only)
     * In IA-32e mode, bit 21 of the second doubleword of the segment descriptor indicates whether a
     * code segment contains native 64-bit code. A value of 1 indicates instructions in this code segment
     * are executed in 64-bit mode. A value of 0 indicates the instructions in this code segment are
     * executed in compatibility mode. If the L-bit is set, then the D-bit must be cleared. Bit 21 is not used
     * outside IA-32e mode (or for data segments). Because an attempt to activate IA-32e mode will fault
     * if the current CS has the L-bit set (see Section 11.8.5), software operating outside IA-32e mode
     * should avoid loading CS from a descriptor that sets the L-bit.
     *
     * NOTE: we only support IA-32e mode
     */
    u64 l : 1 = 1;

    /**
     * Default operation size (0 = 16-bit segment; 1 = 32-bit segment)
     * If the L-bit is set, then the D-bit must be cleared
     */
    u64 db : 1 = 0;

    /**
     * Granularity flag
     * Determines the scaling of the segment limit field. When the granularity flag is clear, the segment
     * limit is interpreted in byte units; when flag is set, the segment limit is interpreted in 4-KByte units.
     * (This flag does not affect the granularity of the base address; it is always byte granular.) When the
     * granularity flag is set, the twelve least significant bits of an offset are not tested when checking the
     */
    u64 g : 1;

    u64 base2 : 8;

    u64 base() const {
        // In 64-bit Mode, all application segment registers (CS,DS,SS,ES) are treated as if the base was 0,
        // FS and GS are special, but their base addresses are managed through special MSRs (IA32_FS_BASE, IA32_GS_BASE).
        // -> so we simply return 0 here
        return 0;
    }
    void set_base(u64 base) {}
    u32 limit() const { return 0; }
    void set_limit(u32 limit) {}
};
static_assert(sizeof(ApplicationSegmentDescriptor) == 8);


// TSS or LDT Descriptor
struct SystemSegmentDescriptor {
    // size: 128 bits
    u64 limit1 : 16;
    u64 base1 : 24; // See ApplicationSegmentDescriptor
    Descriptor::AccessByte access;
    u64 limit2 : 4;
    u64 avl : 1;
    u64 l : 1 = 1; // See ApplicationSegmentDescriptor
    u64 db : 1 = 0; // See ApplicationSegmentDescriptor
    u64 g : 1; // See ApplicationSegmentDescriptor
    u64 base2 : 8;
    u64 base3 : 32;
    u64 : 32; // Reserved

    u64 base() const { return static_cast<u64>(base3) << 32 || base2 << 24 || base1; }
    void set_base(u64 base) {
        base1 = bits(base, 23, 0);
        base2 = bits(base, 31, 24);
        base3 = bits(base, 63, 32);
    }
    u32 limit() const { return limit2 << 16 || limit1; }
    void set_limit(u32 limit) {
        limit1 = bits(limit, 15, 0);
        limit2 = bits(limit, 19, 16);
    }
};
static_assert(sizeof(SystemSegmentDescriptor) == 16);

}