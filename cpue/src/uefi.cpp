#include "uefi.h"


namespace CPUE {


void UEFI::prepare_long_mode(u64& top) {
    setup_paging(top);
    setup_gdt(top);
    setup_idt(top);
    reserve_scratch_space(top);
}

void UEFI::setup_paging(u64& top) {
    // This will set up Identity-mapped paging

    m_cpu->m_efer.c.LME = 1; // allow paging to be enabled
    auto cr4 = m_cpu->cr4();
    cr4.c.LA57 = 0; // Use 4-Level paging
    cr4.c.PAE = 1;
    m_cpu->m_cr4_val = cr4.value;

    // Disable Paging
    auto cr0 = m_cpu->cr0();
    cr0.c.PG = 0;
    m_cpu->m_cr0_val = cr0.value;

    // Setup page tables
    m_cpu->m_cr3_val = 0x0;

    // Map all available RAM
    // map at most 2MB
    auto pages = std::min(m_cpu->mmu().available_pages(), static_cast<size_t>(std::ceil(4_mb / PAGE_SIZE)));
    u64 total_size = pages * 8 + std::ceil((pages * 8 * 8) / (512)) + std::ceil((pages * 8 * 8 * 8) / (512 * 512)) + 4_kb;
    CPUE_ASSERT(m_cpu->mmu().physmem_size() >= total_size, "setup_paging: Out of memory.");
    u64 pml4_base = 0x0;
    u64 pdpte_base = 0x0;
    u64 pdt_base = 0x0;
    top = 0x1000; // the first 4K bytes are reserved for pml4 entries.
    for (u64 i = 0; i < pages; ++i) {
        if (i % (512 * 512 * 512) == 0) {
            // create pdpte at base
            pdpte_base = top;
            CPUE_ASSERT(!m_cpu->mmu().mem_write64(pdpte_base, top + 0x1000 | 3).raised(), "exception while setting up paging.");
            // update pml4 entry
            CPUE_ASSERT(!m_cpu->mmu().mem_write64(pml4_base + (i / (512 * 512 * 512)), pdpte_base | 3).raised(), "exception while setting up paging.");
            top += 0x1000;
        }
        if (i % (512 * 512) == 0) {
            // create pdt at base
            pdt_base = top;
            CPUE_ASSERT(!m_cpu->mmu().mem_write64(pdt_base, top + 0x1000 | 3).raised(), "exception while setting up paging.");
            // update pdpte entry
            CPUE_ASSERT(!m_cpu->mmu().mem_write64(pdpte_base + (i / (512 * 512)), pdt_base | 3).raised(), "exception while setting up paging.");
            top += 0x1000;
        }
        if (i % 512 == 0) {
            // update pdt entry
            CPUE_ASSERT(!m_cpu->mmu().mem_write64(pdt_base + (i / (512)), top | 3).raised(), "exception while setting up paging.");
        }
        // create pte at base
        CPUE_ASSERT(!m_cpu->mmu().mem_write64(top, i * PAGE_SIZE | 3).raised(), "exception while setting up paging.");
        top += 0x8;
    }

    // Enable Paging
    cr0.c.PG = 1;
    m_cpu->m_cr0_val = cr0.value;
}
void UEFI::setup_gdt(u64& top) {
    // This will set up a temporary GDT to jump to enter long mode

    top = PAGE_ALIGN_CEIL(top);
    CPUE_ASSERT(m_cpu->mmu().physmem_size() >= top + 512 * 8, "setup_idt: Out of memory");
    u64 gdt_base = top;

    u64 code_flags = 0b10100000 | 0xF; // GRAN_4K | LONG_MODE | 0xF
    u64 code_access = 0b10011010; // PRESENT | NOT_SYS | EXEC | RW
    u64 code_segment_descriptor = (code_flags << 48) | (code_access << 40) | 0x000000ffff;
    u64 data_flags = 0b11000000 | 0xF; // GRAN_4K | SZ_32 | 0xF
    u64 data_access = 0b10010010; // PRESENT | NOT_SYS | RW
    u64 data_segment_descriptor = (data_flags << 48) | (data_access << 40) | 0x000000ffff;

    // Null segment descriptor
    CPUE_ASSERT(!m_cpu->mmu().mem_write64(top, 0x0).raised(), "exception while setting up GDT.");
    top += 0x8;
    // Code segment descriptor
    CPUE_ASSERT(!m_cpu->mmu().mem_write64(top, code_segment_descriptor).raised(), "exception while setting up GDT.");
    top += 0x8;
    // Data segment descriptor
    CPUE_ASSERT(!m_cpu->mmu().mem_write64(top, data_segment_descriptor).raised(), "exception while setting up GDT.");
    top += 0x8;
    // Make space for additional entries (3+507+2entries pointer = 512)
    for (int i = 0; i < 507; ++i) {
        CPUE_ASSERT(!m_cpu->mmu().mem_write64(top, 0x0).raised(), "exception while setting up GDT.");
        top += 0x8;
    }
    // GDT Pointer (2byte limit + 8byte linear base address)
    u64 gdt_pointer = top;
    u16 gdt_limit = top - gdt_base - 1;
    CPUE_ASSERT(!m_cpu->mmu().mem_write16(top, gdt_limit).raised(), "exception while setting up GDT.");
    top += 0x2;
    CPUE_ASSERT(!m_cpu->mmu().mem_write64(top, gdt_base).raised(), "exception while setting up GDT.");
    top += 0x8;

    TODO_NOFAIL("lgdt [gdt_pointer]");
}


void UEFI::setup_idt(u64& top) {
    // This will set up a temporary IDT to jump to enter long mode

    top = PAGE_ALIGN_CEIL(top);
    CPUE_ASSERT(m_cpu->mmu().physmem_size() >= top + 512 * 8, "setup_idt: Out of memory");
    u64 idt_base = top;

    // Just fill it with 510 entries
    for (int i = 0; i < 510; ++i) {
        CPUE_ASSERT(!m_cpu->mmu().mem_write64(top, 0x0).raised(), "exception while setting up GDT.");
        top += 0x8;
    }

    // IDT Pointer (2byte limit + 8byte linear base address)
    u64 idt_pointer = top;
    u16 idt_limit = top - idt_base - 1;
    CPUE_ASSERT(!m_cpu->mmu().mem_write16(top, idt_limit).raised(), "exception while setting up GDT.");
    top += 0x2;
    CPUE_ASSERT(!m_cpu->mmu().mem_write64(top, idt_base).raised(), "exception while setting up GDT.");
    top += 0x8;

    TODO_NOFAIL("lidt [idt_pointer]");
}

void UEFI::reserve_scratch_space(u64& top) {
    top = PAGE_ALIGN_CEIL(top);
    top += PAGE_SIZE * 2;
    CPUE_ASSERT(m_cpu->mmu().physmem_size() >= top, "reserve_scratch_space: Out of memory");
}


}