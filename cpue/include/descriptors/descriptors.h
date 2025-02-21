#pragma once

#include <concepts>
#include <utility>
#include "common.h"

namespace CPUE {


template<typename T>
concept DescriptorWithBaseAddress = requires(T t) {
    { t.base() } -> std::convertible_to<u64>;
    { t.set_base(std::declval<u64>()) } -> std::same_as<void>;
};
template<typename T>
concept DescriptorWithNoBaseAddress = !DescriptorWithBaseAddress<T>;
template<typename T>
concept DescriptorWithLimit = requires(T t) {
    { t.limit() } -> std::same_as<u32>;
    { t.set_limit(std::declval<u32>()) } -> std::same_as<void>;
};
template<typename T>
concept DescriptorWithNoLimit = !DescriptorWithLimit<T>;
template<typename T>
concept DescriptorWithOffset = requires(T t) {
    { t.offset() } -> std::same_as<u64>;
    { t.set_offset(std::declval<u64>()) } -> std::same_as<void>;
};
template<typename T>
concept DescriptorWithNoOffset = !DescriptorWithOffset<T>;
template<typename T>
concept DescriptorWithSegmentSelector = requires(T t) {
    { t.segment_selector } -> std::same_as<SegmentSelector>;
};
template<typename T>
concept DescriptorWithNoSegmentSelector = !DescriptorWithSegmentSelector<T>;


enum DescriptorType {
    RESERVED,

    // Application Descriptors
    DATA_SEGMENT, // 64 bits
    CODE_SEGMENT, // 64 bits

    // System Descriptors
    LDT_SEGMENT, // 128 bits
    TSS_AVAILABLE_SEGMENT, // 128 bits
    TSS_BUSY_SEGMENT, // 128 bits
    /**
     * NOTE: Task-switching is not supported in 64-bit mode, we list it here for completion.
     * Task management and switching must be performed by software. The processor issues a general-protection
     * exception (#GP) if the following is attempted in 64-bit mode:
     * - Control transfer to a TSS or a task gate using JMP, CALL, INT n, INT3, INTO, INT1, or interrupt.
     * - An IRET with EFLAGS.NT (nested task) set to 1.
     */
    TASK_GATE, // 64 bits
    CALL_GATE, // 128 bits
    INTERRUPT_GATE, // 128 bits
    TRAP_GATE, // 128 bits
};

/**
 * This is only the type map for IA-32e mode. That is, in particular the
 * 16/32 bit versions of System-Segment/Gate-Descriptors and the Task-Gate
 * types are not supported. When trying to access a RESERVED Descriptor, the processor
 * will issue a #GP exception.
 */
constexpr DescriptorType _system_descriptor_type_map[16] = {
    /**
     * Type = 0b0000 is RESERVED, because this will prevent unaligned access into the GDT/LDT/IDT
     * when the entry is not initialized, or it is the higher qword of an 16-byte entry
     * (these set type=0b0000 in their higher qword).
     */
    DescriptorType::RESERVED,
    DescriptorType::RESERVED,
    DescriptorType::LDT_SEGMENT,
    DescriptorType::RESERVED,
    DescriptorType::RESERVED,
    DescriptorType::RESERVED,
    DescriptorType::RESERVED,
    DescriptorType::RESERVED,
    DescriptorType::RESERVED,
    DescriptorType::TSS_AVAILABLE_SEGMENT,
    DescriptorType::RESERVED,
    DescriptorType::TSS_BUSY_SEGMENT,
    DescriptorType::CALL_GATE,
    DescriptorType::RESERVED,
    DescriptorType::INTERRUPT_GATE,
    DescriptorType::TRAP_GATE,
};


struct DescriptorTable {
    u64 base; // The base address specifies the linear address of byte 0 of the IDT
    u16 limit; // The table limit specifies the number of bytes in the table
};


struct Descriptor {
    // size: 64 bits

    // Descriptor-Type specific
    u64 : 32;
    u64 : 8;

    union AccessByte {
        struct Concrete {
            /**
             * Type field
             * Indicates the segment or gate type and specifies the kinds of access that can be made to the
             * segment and the direction of growth. The interpretation of this field depends on whether the
             * descriptor type flag specifies an application (code or data) descriptor or a system descriptor.
             * Stack segments are data segments which must be read/write segments.
             */
            u8 accessed : 1; // The processor sets this bit whenever it loads a segment selector for the segment into a segment register, assuming that the type of memory that contains the segment descriptor supports processor writes. The bit remains set until explicitly cleared. This bit can be used both for virtual memory management and for debugging.
            u8 wr : 1; // Data: Write, Code: Read
            /**
             * Code segments can be either conforming (bit set) or nonconforming (bit clear). A transfer of execution into a more-privileged
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

            /**
             * S (descriptor type) flag
             * Specifies whether the segment descriptor is for a system segment (S flag is clear) or a code or data
             * segment (S flag is set).
             */
            u8 no_system_segment : 1;

            // Descriptor Privilege Level
            u8 dpl : 2;

            /**
             * Present flag
             * Indicates whether the segment is present in memory (set) or not present (clear).
             * If this flag is clear, the processor generates a segment-not-present exception (#NP)
             * when a segment selector that points to the segment descriptor is loaded into a segment register.
             * Memory management software can use this flag to control which segments are actually loaded into physical memory at a given time.
             *
             * For call gates this means: gate valid(set) or gate invalid (clear)
             * -> The presence of the code segment to which the gate points is indicated by the P flag in the code segment’s descriptor.
             */
            u8 present : 1;
        } c;
        u8 value;

        bool is_application_descriptor() const { return c.no_system_segment; }
        bool is_system_descriptor() const { return !is_application_descriptor(); }
        // return true if the descriptor has an expanded form (is 128-bits wide)
        bool is_expanded_descriptor() const {
            return one_of(descriptor_type(), {DescriptorType::LDT_SEGMENT, DescriptorType::TSS_AVAILABLE_SEGMENT, DescriptorType::TSS_BUSY_SEGMENT, DescriptorType::INTERRUPT_GATE,
                                                 DescriptorType::TRAP_GATE, DescriptorType::CALL_GATE});
        }

        u8 type_value() const { return c.executable << 3 | c.ec << 2 | c.wr << 1 | c.accessed; }

        DescriptorType descriptor_type() const {
            if (is_system_descriptor())
                return _system_descriptor_type_map[type_value()];
            if (c.executable)
                return DescriptorType::CODE_SEGMENT;
            return DescriptorType::DATA_SEGMENT;
        }
    } access;

    // Descriptor-Type specific
    u64 : 16;
};
static_assert(sizeof(Descriptor) == 8);




}