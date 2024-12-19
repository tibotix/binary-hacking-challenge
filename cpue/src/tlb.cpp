

#include "tlb.h"
#include "cpu.h"

namespace CPUE {


constexpr u64 TLB::key(VirtualAddress const& vaddr) const {
    return (vaddr.addr & PAGE_NUMBER_MASK);
}

std::optional<TLBEntry> TLB::lookup(VirtualAddress const& vaddr) const {
    bool allow_global_pages = m_cpu->m_cr4.c.PGE == 1;
    auto range = m_entries.equal_range(key(vaddr));
    for (auto it = range.first; it != range.second; ++it) {
        // if we find entry with current pcid, take it
        if (it->second.pcid == m_cpu->pcid())
            return it->second;
        // A logical processor may use a global TLB entry to translate a linear address, even if the TLB entry is associated with
        // a PCID different from the current PCID.
        if (allow_global_pages && it->second.global)
            return it->second;
    }
    return {};
}

void TLB::insert(VirtualAddress const& vaddr, TLBEntry const& entry) {
    if (m_entries.size() == TLB_NUM_ENTRIES) {
        m_entries.erase(m_entries.begin());
    }
    m_entries.insert({key(vaddr), entry});
}


void TLB::invalidate(VirtualAddress const& vaddr) {
    auto range = m_entries.equal_range(key(vaddr));
    if (range.first == m_entries.end())
        return;
    m_entries.erase(range.first, range.second);
}

void TLB::invalidate_all() {
    m_entries.clear();
}

}