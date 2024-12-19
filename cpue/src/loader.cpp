#include "loader.h"


namespace CPUE {


void Loader::load_region(Region const& region, u64& top) {
    // create page table structures for region VAS.
    create_region_vas(region, top);

    TranslationContext ctx = {
        .op = MemoryOp::OP_WRITE,
        .width = ByteWidth::WIDTH_QWORD,
        .intention = TranslationIntention::INTENTION_UNKNOWN,
    };

    auto page_count = region.get_page_count();
    for (u64 i = 0; i < page_count; ++i) {
        // TODO: Use page_table_walk API to get pointer to PTE or make new API va_to_pte_no_tlb()
        auto result = m_cpu.mmu().va_to_pte_no_tlb(region.base + i * PAGE_SIZE, ctx);
        CPUE_ASSERT(!result.raised(), "load_region: critical fault during loading of region.");
        auto* pte = result.release_value();
        pte->c.paddr = top >> 12;
        CPUE_ASSERT(m_cpu.mmu().physmem_size() >= top + PAGE_SIZE, "load_region: out-of-memory while loading of region data.");
        // TODO: Or use mem_write API from MMU (much slower, but tests correctness of PageTables)
        if (region.data)
            memcpy(m_cpu.mmu().paddr_ptr(top), region.data + i * PAGE_SIZE, PAGE_SIZE);
        top += PAGE_SIZE;
    }
    // TODO: maybe make public getter for tlb and maybe don't invalidate
    m_cpu.mmu().m_tlb.invalidate_all();
};

void Loader::create_region_vas(Region const& region, u64& top) {
    //    CPUE_ASSERT(m_cpu.is_paging_enabled(), "Paging must be enabled for something to be loaded into VAS.");
    // TODO: add initial_paddr strategy so we can easily identity-map with this function

    auto page_count = region.get_page_count();
    for (u64 i = 0; i < page_count; ++i) {
        // Do page table walk.
        auto result = m_cpu.mmu().page_table_walk(region.base + i * PAGE_SIZE, [](PageStructureEntry* entry, void* data) -> bool {
            // If either entry is empty or just accessed bit is set, we consider it as not already mapped -> exit walk and map it.
            if (entry->value == 0x0 || entry->value == 0x20)
                return false;
            return true;
        });

        if (result.aborted) {
            // We need to map the faulty entry
            // Somewhere here check if physmem_size >= top+...
            return;
        }
    }
#if 0
    // Map all available RAM
    // map at most 2MB
    auto pages = std::min(m_cpu.mmu().available_pages(), static_cast<size_t>(std::ceil(4_mb / PAGE_SIZE)));
    u64 total_size = pages * 8 + std::ceil((pages * 8 * 8) / (512)) + std::ceil((pages * 8 * 8 * 8) / (512 * 512)) + 4_kb;
    CPUE_ASSERT(m_cpu.mmu().physmem_size() >= total_size, "setup_paging: Out of memory.");
    u64 pml4_base = 0x0;
    u64 pdpte_base = 0x0;
    u64 pdt_base = 0x0;
    top = 0x1000; // the first 4K bytes are reserved for pml4 entries.
    for (u64 i = 0; i < pages; ++i) {
        if (i % (512 * 512 * 512) == 0) {
            // create pdpte at base
            pdpte_base = top;
            CPUE_ASSERT(!m_cpu.mmu().mem_write64(pdpte_base, top + 0x1000 | 3).raised(), "exception while setting up paging.");
            // update pml4 entry
            CPUE_ASSERT(!m_cpu.mmu().mem_write64(pml4_base + (i / (512 * 512 * 512)), pdpte_base | 3).raised(), "exception while setting up paging.");
            top += 0x1000;
        }
        if (i % (512 * 512) == 0) {
            // create pdt at base
            pdt_base = top;
            CPUE_ASSERT(!m_cpu.mmu().mem_write64(pdt_base, top + 0x1000 | 3).raised(), "exception while setting up paging.");
            // update pdpte entry
            CPUE_ASSERT(!m_cpu.mmu().mem_write64(pdpte_base + (i / (512 * 512)), pdt_base | 3).raised(), "exception while setting up paging.");
            top += 0x1000;
        }
        if (i % 512 == 0) {
            // update pdt entry
            CPUE_ASSERT(!m_cpu.mmu().mem_write64(pdt_base + (i / (512)), top | 3).raised(), "exception while setting up paging.");
        }
        // create pte at base
        CPUE_ASSERT(!m_cpu.mmu().mem_write64(top, i * PAGE_SIZE | 3).raised(), "exception while setting up paging.");
        top += 0x8;
    }
#endif
};


}