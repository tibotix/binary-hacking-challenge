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
        load_region(r, top, !user_elf);
    }
}

void Loader::load_region(Region const& region, u64& top, bool allow_overwrite) {
    CPUE_ASSERT(IS_PAGE_ALIGNED(top), "top is not page aligned.");

    // create page table structures for region VAS.
    if (!create_region_vas(region, top) && !allow_overwrite)
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

    CPUE_TRACE("create_region_vas for range: 0x{:x} - 0x{:x}", PAGE_ALIGN(region.base.addr), PAGE_ALIGN_CEIL(region.base.addr + region.size));

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
                created_new_ptes = true;
                continue;
            }

            // The page table walk aborted and not all page-table-structures are mapped, so we need to map the faulty entry
            CPUE_ASSERT(m_cpu.mmu().physmem_size() >= top + 0x1000, "create_region_vas: Out-of-Memory.");
            auto table_base = m_cpu.mmu().paddr_ptr<u64>(top);
            memset(table_base, 0, 0x1000);

            CPUE_ASSERT(result.pml4e != nullptr, "create_region_vas: pml4e == nullptr.");
            if (result.pdpte == nullptr) {
                CPUE_TRACE("Creating new pdpt @ 0x{:x}", top);
                // update pml4 entry
                result.pml4e->value = top | 7;
                top += 0x1000;
            } else if (result.pde == nullptr) {
                CPUE_TRACE("Creating new page directory @ 0x{:x}", top);
                // update pdpte entry
                result.pdpte->value = top | 7;
                top += 0x1000;
            } else if (result.pte == nullptr) {
                CPUE_TRACE("Creating new page table @ 0x{:x}", top);
                // update pde entry
                result.pde->value = top | 7;
                top += 0x1000;
            }
        }
    }

    return created_new_ptes;
};


}