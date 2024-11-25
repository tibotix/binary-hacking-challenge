#pragma once

#include "forward.h"
#include "paging.h"

#include <address.h>
#include <optional>
#include <unordered_map>

namespace CPUE {

/**
 * Translation Lookaside Buffers (TLBs) (See page 3231)
 *
 * The processor may accelerate the paging process by caching individual translations in translation lookaside
 * buffers (TLBs). Each entry in a TLB is an individual translation. Each translation is referenced by a page number.
 *
 * If the translation does use a PTE, the page size is 4 KBytes and the page number comprises bits 47:12 of
 * the linear address.
 *
 * If the page number of a linear address corresponds to a TLB entry associated with the current PCID, the processor
 * may use that TLB entry to determine the page frame, access rights, and other attributes for accesses to that linear
 * address. In this case, the processor may not actually consult the paging structures in memory. The processor may
 * retain a TLB entry unmodified even if software subsequently modifies the relevant paging-structure entries in
 * memory. See Section 5.10.4.2 for how software can ensure that the processor uses the modified paging-structure
 * entries.
 */

constexpr size_t TLB_NUM_ENTRIES = 256;
static_assert(TLB_NUM_ENTRIES > 0, "TLB_NUM_ENTRIES must be greater than zero");


struct TLBEntry {
    // size: 64 bits
    u64 paddr : MAXPHYADDR - 12;
    u64 rw : 1; // anded
    u64 user : 1; // anded
    u64 xd : 1; // ored
    u64 dirty : 1;
    u64 global : 1;
    u64 pkey : 4;
    u64 pcid : 12;
    u64 : 55 - MAXPHYADDR; // padding to 64bits
};
static_assert(sizeof(TLBEntry) == 8);

class TLB {
public:
    explicit TLB(CPU* cpu) : m_cpu(cpu) { m_entries.reserve(TLB_NUM_ENTRIES); };
    TLB(TLB const&) = delete;

public:
    [[nodiscard]] std::optional<TLBEntry> lookup(VirtualAddress const& vaddr) const;
    void insert(VirtualAddress const& vaddr, TLBEntry const& entry);
    void invalidate(VirtualAddress const& vaddr);
    void invalidate_all();

private:
    constexpr u64 key(VirtualAddress const& vaddr) const;

private:
    std::unordered_multimap<u64, TLBEntry> m_entries;
    CPU* m_cpu;
};


}