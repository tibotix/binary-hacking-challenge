#pragma once

#include "common.h"
#include "segment_register.h"

#include <checked_arithmetic.h>

namespace CPUE {




/**
 * A LogicalAddress consists of a segment and an offset based on the segment.
 * It is subject to Segmentation and produces a VirtualAddress if paging enabled, otherwise a PhysicalAddress.
 *
 * NOTE: We store the SegmentRegister instead of the SegmentSelector, as we want to pass the cached "hidden" part
 * of the SegmentRegister to MMU Translation as well.
 * And as every LogicalAddress Translation can only use an already loaded segment register, this is justified.
 */
struct LogicalAddress {
    ApplicationSegmentRegister segment_register;
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

    VirtualAddress operator+(unsigned long int const i) const { return {CPUE_checked_uadd<u64, u64>(addr, i)}; }
    VirtualAddress& operator+=(unsigned long int const i) {
        addr = CPUE_checked_uadd<u64, u64>(addr, i);
        return *this;
    }
    VirtualAddress operator-(unsigned long int const i) const { return {CPUE_checked_usub<u64, u64>(addr, i)}; }
    VirtualAddress& operator-=(unsigned long int const i) {
        addr = CPUE_checked_usub<u64, u64>(addr, i);
        return *this;
    }
    bool operator<(VirtualAddress const& other) const { return addr < other.addr; }
    bool operator<=(VirtualAddress const& other) const { return addr <= other.addr; }
    bool operator>(VirtualAddress const& other) const { return addr > other.addr; }
    bool operator>=(VirtualAddress const& other) const { return addr >= other.addr; }
    bool operator==(VirtualAddress const& other) const { return addr == other.addr; }
};

/**
 * A PhysicalAddress is the absolute address in the physical memory.
 * It was obtained by Segmentation, and if paging enabled, also Paging.
 */
struct PhysicalAddress {
    PhysicalAddress() = default;
    constexpr PhysicalAddress(u64 addr) : addr(addr) {}
    u64 addr;

    PhysicalAddress operator+(unsigned long int const i) const { return {CPUE_checked_uadd<u64, u64>(addr, i)}; }
    PhysicalAddress& operator+=(unsigned long int const i) {
        addr = CPUE_checked_uadd<u64, u64>(addr, i);
        return *this;
    }
    PhysicalAddress operator-(unsigned long int const i) const { return {CPUE_checked_usub<u64, u64>(addr, i)}; }
    PhysicalAddress& operator-=(unsigned long int const i) {
        addr = CPUE_checked_usub<u64, u64>(addr, i);
        return *this;
    }
    bool operator<(PhysicalAddress const& other) const { return addr < other.addr; }
    bool operator<=(PhysicalAddress const& other) const { return addr <= other.addr; }
    bool operator>(PhysicalAddress const& other) const { return addr > other.addr; }
    bool operator>=(PhysicalAddress const& other) const { return addr >= other.addr; }
    bool operator==(PhysicalAddress const& other) const { return addr == other.addr; }
};


constexpr VirtualAddress operator""_va(unsigned long long vaddr) {
    return VirtualAddress{vaddr};
}
constexpr PhysicalAddress operator""_pa(unsigned long long paddr) {
    return PhysicalAddress{paddr};
}




}