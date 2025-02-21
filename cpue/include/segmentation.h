#pragma once

#include "common.h"

namespace CPUE {


/**
 * Segmentation in IA-32e Mode:
 * In IA-32e mode of Intel 64 architecture, the effects of segmentation depend on whether the processor is running
 * in compatibility mode or 64-bit mode. In compatibility mode, segmentation functions just as it does using legacy
 * 16-bit or 32-bit protected mode semantics.
 * In 64-bit mode, segmentation is generally (but not completely) disabled, creating a flat 64-bit linear-address
 * space. The processor treats the segment base of CS, DS, ES, SS as zero, creating a linear address that is equal to
 * the effective address. The FS and GS segments are exceptions. These segment registers (which hold the segment
 * base) can be used as additional base registers in linear address calculations. They facilitate addressing local data
 * and certain operating system data structures.
 * Note that the processor does not perform segment limit checks at runtime in 64-bit mode.
 */


union SegmentSelector {
    // size: 16 bits
    constexpr SegmentSelector() = default;
    constexpr SegmentSelector(const SegmentSelector&) = default;
    constexpr SegmentSelector(u16 value) : value(value){};
    constexpr SegmentSelector(u16 rpl, u8 table, u8 index) : c({.rpl = rpl, .table = table, .index = index}) {}
    struct Concrete {
        u16 rpl : 2; // Requested Privilege Level
        u16 table : 1; // Table Indicator (0=GDT,1=LDT)
        /**
         * Selects one of 8192 descriptors in the GDT or LDT.
         * The processor multiplies the index value by 8 and adds the result to the
         * base address of the GDT or LDT (from the GDTR or LDTR register, respectively)
         */
        u16 index : 13;
    } c;
    u16 value;
};
static_assert(sizeof(SegmentSelector::Concrete) == 2);
static_assert(sizeof(SegmentSelector) == 2);




}