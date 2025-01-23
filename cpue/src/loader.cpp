#include "loader.h"
#include "elf.h"

#include "logging.h"

namespace CPUE {


void Loader::load_elf(ELF* elf, u64& top, bool user_elf) {
    CPUE_ASSERT(elf->is_elf64(), "ELF is not 64-bit executable.");
    CPUE_ASSERT(!elf->is_pie(), "ELF is a PIE, but we only allow non-pie binaries.");
    CPUE_ASSERT(elf->is_executable(), "ELF is not an executable.");
    CPUE_ASSERT(elf->is_static(), "ELF is not statically compiled.");
    for (auto& region : elf->regions()) {
        Region r = region;
        if (user_elf)
            r.flags |= REGION_USER;
        load_region(r, top);
    }
}

void Loader::load_region(Region const& region, u64& top) {
    CPUE_ASSERT(IS_PAGE_ALIGNED(top), "top is not page aligned.");

    // create page table structures for region VAS.
    if (!create_region_vas(region, top))
        fail("Trying to load a Region into already mapped VAS.");

    TranslationContext ctx = {
        .op = MemoryOp::OP_WRITE,
        .width = ByteWidth::WIDTH_QWORD,
        .intention = TranslationIntention::INTENTION_UNKNOWN,
    };

    auto page_count = region.get_page_count();
    u64 bytes_copied = 0;
    for (u64 i = 0; i < page_count; ++i) {
        auto result = m_cpu.mmu().va_to_pte_no_tlb(region.base + i * PAGE_SIZE, ctx);
        CPUE_ASSERT(!result.raised(), "load_region: critical fault during loading of region.");
        auto* pte = result.release_value();
        pte->c.paddr = top >> 12;
        CPUE_ASSERT(m_cpu.mmu().physmem_size() >= top + PAGE_SIZE, "load_region: out-of-memory while loading of region data.");
        // TODO: Or use mem_write API from MMU (much slower, but tests correctness of PageTables)
        if (region.data) {
            auto bytes_to_copy = std::min(region.size - bytes_copied, PAGE_SIZE);
            memcpy(m_cpu.mmu().paddr_ptr(top), region.data + bytes_copied, bytes_to_copy);
            bytes_copied += bytes_to_copy;
        }
        top += PAGE_SIZE;
    }
};

bool Loader::create_region_vas(Region const& region, u64& top, u64 (*initial_mapping_strategy)(u64)) {
    CPUE_ASSERT(IS_PAGE_ALIGNED(top), "top is not page aligned.");

    bool created_new_ptes = false;

    auto page_count = region.get_page_count();
    for (u64 i = 0; i < page_count; ++i) {
        auto current_page = region.base + i * PAGE_SIZE;
        // Do page table walk.
        while (true) {
            auto result = m_cpu.mmu().page_table_walk(current_page, [](PageStructureEntry* entry, void* data) -> bool {
                // If either entry is empty or just accessed bit is set, we consider it as not already mapped -> exit walk and map it.
                if (entry->value == 0x0 || entry->value == 0x20)
                    return false;
                return true;
            });

            // If page table walk was not aborted, we found a valid PTE -> break loop
            if (!result.aborted) {
                break;
            }

            if (result.pte != nullptr) {
                // we found a pte, but it is still zero
                // -> initialize page frame number with given strategy.
                result.pte->value = initial_mapping_strategy(current_page.addr & PAGE_NUMBER_MASK) | region.flags | 1;
                continue;
            }

            // The page table walk aborted and not all page-table-structures are mapped, so we need to map the faulty entry
            CPUE_ASSERT(m_cpu.mmu().physmem_size() >= top + 0x1000, "create_region_vas: Out-of-Memory.");
            auto table_base = m_cpu.mmu().paddr_ptr<u64>(top);
            memset(table_base, 0, 0x1000);

            CPUE_ASSERT(result.pml4e != nullptr, "create_region_vas: pml4e == nullptr.");
            if (result.pdpte == nullptr) {
                // update pml4 entry
                result.pml4e->value = top | 7;
                top += 0x1000;
            } else if (result.pde == nullptr) {
                // update pdpte entry
                result.pdpte->value = top | 7;
                top += 0x1000;
            } else if (result.pte == nullptr) {
                // update pde entry
                result.pde->value = top | 7;
                top += 0x1000;
            }
            created_new_ptes = true;
        }
    }

    return created_new_ptes;

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