#pragma once

#include "common.h"

namespace CPUE {

struct Segment {};


struct SegmentSelector {
    constexpr SegmentSelector(u16 value) : rpl(bits(value, 1, 0)), table(bits(value, 2, 2)), index(bits(value, 15, 3)) {}
    constexpr SegmentSelector(u16 rpl, u8 table, u8 index) : rpl(rpl), table(table), index(index) {}
    u16 rpl : 2; // Requested Privilege Level
    u8 table : 1; // Table Indicator (0=GDT,1=LDT)
    /**
     * Selects one of 8192 descriptors in the GDT or LDT.
     * The processor multiplies the index value by 8 (the number of bytes in a segment descriptor) and adds the result to the
     * base address of the GDT or LDT (from the GDTR or LDTR register, respectively)
     */
    u16 index : 13;
};


/**
 * A LogicalAddress consists of a segment and an offset based on the segment.
 * It is subject to Segmentation and produces a VirtualAddress if paging enabled, otherwise a PhysicalAddress.
 */
struct LogicalAddress {
    SegmentSelector segment_selector;
    u64 offset;
};

/**
 * A VirtualAddress (also named LinearAddress) only contains the absolute addr.
 * That addr is still subject to paging.
 * If paging is not used, the processor maps the linear address directly to a physical address (that is, the linear
 * address goes out on the processor’s address bus). If the linear address space is paged, a second level of address
 * translation is used to translate the linear address into a physical address (paging).
 */
struct VirtualAddress {
    VirtualAddress() = default;
    constexpr VirtualAddress(u64 addr) : addr(addr) {}
    u64 addr;
};

/**
 * A PhysicalAddress is the absolute address in the physical memory.
 * It was obtained by Segmentation, and if paging enabled, also Paging.
 */
struct PhysicalAddress {
    PhysicalAddress() = default;
    constexpr PhysicalAddress(u64 addr) : addr(addr) {}
    u64 addr;
};


constexpr VirtualAddress operator""_va(unsigned long long vaddr) {
    return VirtualAddress{vaddr};
}
constexpr PhysicalAddress operator""_pa(unsigned long long paddr) {
    return PhysicalAddress{paddr};
}




}