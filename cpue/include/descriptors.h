#pragma once

#include "common.h"

namespace CPUE {



enum SegmentType {
    DATA,
    CODE,
    SYSTEM,
};

// We only implement IA-32e Mode Types
enum SystemDescriptorType {
    RESERVED,
    LDT,
    TSS_AVAILABLE,
    TSS_BUSY,
    CALL_GATE,
    INTERRUPT_GATE,
    TRAP_GATE,
};

SystemDescriptorType _system_descriptor_type_map[16] = {
    SystemDescriptorType::RESERVED,
    SystemDescriptorType::RESERVED,
    SystemDescriptorType::LDT,
    SystemDescriptorType::RESERVED,
    SystemDescriptorType::RESERVED,
    SystemDescriptorType::RESERVED,
    SystemDescriptorType::RESERVED,
    SystemDescriptorType::RESERVED,
    SystemDescriptorType::RESERVED,
    SystemDescriptorType::TSS_AVAILABLE,
    SystemDescriptorType::RESERVED,
    SystemDescriptorType::TSS_BUSY,
    SystemDescriptorType::CALL_GATE,
    SystemDescriptorType::RESERVED,
    SystemDescriptorType::INTERRUPT_GATE,
    SystemDescriptorType::TRAP_GATE,
};

// See chapter 3.4.5 (page 3174)
struct SegmentDescriptor {
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
    u32 base1 : 24;


    /**
     * Type field
     * Indicates the segment or gate type and specifies the kinds of access that can be made to the
     * segment and the direction of growth. The interpretation of this field depends on whether the
     * descriptor type flag specifies an application (code or data) descriptor or a system descriptor. 
     * Stack segments are data segments which must be read/write segments.
     */
    union _Type {
        struct {
            u8 accessed : 1 = 1; // The processor sets this bit whenever it loads a segment selector for the segment into a segment register, assuming that the type of memory that contains the segment descriptor supports processor writes. The bit remains set until explicitly cleared. This bit can be used both for virtual memory management and for debugging.
            u8 wr : 1; // Data: Write, Code: Read
            /**
              * Code segments can be either conforming or nonconforming. A transfer of execution into a more-privileged
              * conforming segment allows execution to continue at the current privilege level. A transfer into a nonconforming
              * segment at a different privilege level results in a general-protection exception (#GP), unless a call gate or task gate
              * is used (see Section 6.8.1, “Direct Calls or Jumps to Code Segments,” for more information on conforming and
              * nonconforming code segments). System utilities that do not access protected facilities and handlers for some types
              * of exceptions (such as, divide error or overflow) may be loaded in conforming code segments. Utilities that need to
              * be protected from less privileged programs and procedures should be placed in nonconforming code segments.
              * 
              * All data segments are nonconforming, meaning that they cannot be accessed by less privileged programs or procedures
              * (code executing at numerically higher privilege levels).
              * Unlike code segments, however, data segments can be accessed by more privileged programs or procedures (code executing at numerically lower privilege levels)
              * without using a special access gate.
              */
            u8 ec : 1; // Data: expand-down, Code: conforming
            u8 executable : 1; // 1=code segment; 0=data_segment
        } code_or_data_type;
        u8 value : 4;
    } type;

    /**
     * S (descriptor type) flag
     * Specifies whether the segment descriptor is for a system segment (S flag is clear) or a code or data 
     * segment (S flag is set).
     */
    u8 no_system_segment : 1;
    u8 dpl : 2; // Descriptor Privilege Level

    /**
     * Present flag
     * Indicates whether the segment is present in memory (set) or not present (clear).
     * If this flag is clear, the processor generates a segment-not-present exception (#NP)
     * when a segment selector that points to the segment descriptor is loaded into a segment register.
     * Memory management software can use this flag to control which segments are actually loaded into physical memory at a given time.
     */
    u8 present : 1;
    u8 limit2 : 4;
    u8 avl : 1; // Available for use by system software

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
    u8 l : 1 = 1;
    /**
     * Default operation size (0 = 16-bit segment; 1 = 32-bit segment)
     * If the L-bit is set, then the D-bit must be cleared
     */
    u8 db : 1 = 0;

    /**
     * Granularity flag
     * Determines the scaling of the segment limit field. When the granularity flag is clear, the segment
     * limit is interpreted in byte units; when flag is set, the segment limit is interpreted in 4-KByte units.
     * (This flag does not affect the granularity of the base address; it is always byte granular.) When the
     * granularity flag is set, the twelve least significant bits of an offset are not tested when checking the 
     */
    u8 g : 1;
    u64 base2 : 40;
    u32 : 32; // Reserved

    u64 base() { return base2 << 24 || base1; }
    void set_base(u64 base) {
        base1 = bits(base, 23, 0);
        base2 = bits(base, 63, 24);
    }
    u32 limit() { return limit2 << 16 || limit1; }
    void set_limit(u32 limit) {
        limit1 = bits(limit, 15, 0);
        limit2 = bits(limit, 19, 16);
    }

    SegmentType segment_type() {
        if (no_system_segment == 0)
            return SegmentType::SYSTEM;
        if (type.code_or_data_type.executable)
            return SegmentType::CODE;
        return SegmentType::DATA;
    }

    SystemDescriptorType system_descriptor_type() {
        CPUE_ASSERT(no_system_segment == 0, "Descriptor is no system descriptor");
        return _system_descriptor_type_map[type.value];
    }
};


struct TSSDescriptor {};
struct LDTDescriptor {};


}