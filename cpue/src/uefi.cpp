#include "uefi.h"


namespace CPUE {


void UEFI::prepare_long_mode(u64& top) {
    print_sys_info();
    setup_paging(top);
    setup_gdt(top);
    setup_idt(top);
    setup_stack(top);
    reserve_scratch_space(top);
}

void UEFI::print_sys_info() {
    CPUE_INFO("System Information:");
    CPUE_INFO("Available RAM: 0x{:x}B ({}MB)", m_cpu->mmu().available_pages() * PAGE_SIZE, (m_cpu->mmu().available_pages() * PAGE_SIZE) / 1024 / 1024);
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

    // Make space for PML4
    top += 0x1000;

    // Identity map all available RAM, but at most 4MB
    auto pages = std::min(m_cpu->mmu().available_pages(), static_cast<size_t>(std::ceil(4_mb / PAGE_SIZE)));
    CPUE_TRACE("We have {} available 4KB Pages -> map {} of them.", m_cpu->mmu().available_pages(), pages);
    u64 total_size = pages * PAGE_SIZE;
    m_loader.create_region_vas({.base = 0x0, .size = total_size, .flags = REGION_WRITABLE, .data = nullptr}, top, Loader::MappingStrategies::identity);

    // Enable Paging
    cr0.c.PG = 1;
    m_cpu->m_cr0_val = cr0.value;
}
void UEFI::setup_gdt(u64& top) {
    // This will set up a temporary GDT to use in long mode

    top = 0x4000;
    CPUE_ASSERT(m_cpu->mmu().physmem_size() >= top + 512 * 8, "setup_idt: Out of memory");
    u64 gdt_base = top;

    u64 code_flags = 0b10100000 | 0xF; // GRAN_4K | LONG_MODE | 0xF
    u64 ring0_code_access = 0b10011010; // PRESENT | NOT_SYS | EXEC | RW - DPL: 0x0
    u64 ring0_code_segment_descriptor = (code_flags << 48) | (ring0_code_access << 40) | 0x000000ffff; // Base: 0x0 - Limit: 0xffff
    u64 ring3_code_access = 0b11111010; // PRESENT | NOT_SYS | EXEC | RW - DPL: 0x11
    u64 ring3_code_segment_descriptor = (code_flags << 48) | (ring3_code_access << 40) | 0x000000ffff; // Base: 0x0 - Limit: 0xffff
    u64 data_flags = 0b11000000 | 0xF; // GRAN_4K | SZ_32 | 0xF
    u64 ring0_data_access = 0b10010010; // PRESENT | NOT_SYS | RW - DPL: 0x0
    u64 ring0_data_segment_descriptor = (data_flags << 48) | (ring0_data_access << 40) | 0x000000ffff; // Base: 0x0 - Limit: 0xffff
    u64 ring3_data_access = 0b11110010; // PRESENT | NOT_SYS | RW - DPL: 0x11
    u64 ring3_data_segment_descriptor = (data_flags << 48) | (ring3_data_access << 40) | 0x000000ffff; // Base: 0x0 - Limit: 0xffff

    // Null segment descriptor
    CPUE_ASSERT(!m_cpu->mmu().mem_write64(top, 0x0).raised(), "exception while setting up GDT.");
    top += 0x8;
    // Ring0 Code segment descriptor
    CPUE_ASSERT(!m_cpu->mmu().mem_write64(top, ring0_code_segment_descriptor).raised(), "exception while setting up GDT.");
    top += 0x8;
    // Ring0 Data/Stack segment descriptor
    CPUE_ASSERT(!m_cpu->mmu().mem_write64(top, ring0_data_segment_descriptor).raised(), "exception while setting up GDT.");
    top += 0x8;
    // Ring3 Code segment descriptor
    CPUE_ASSERT(!m_cpu->mmu().mem_write64(top, ring3_code_segment_descriptor).raised(), "exception while setting up GDT.");
    top += 0x8;
    // Ring3 Data/Stack segment descriptor
    CPUE_ASSERT(!m_cpu->mmu().mem_write64(top, ring3_data_segment_descriptor).raised(), "exception while setting up GDT.");
    top += 0x8;
    // Make space for additional entries (5+505+2entries pointer = 512)
    for (int i = 0; i < 505; ++i) {
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

    // TODO: maybe use actual lgdt [gdt_pointer] insn
    m_cpu->m_gdtr = {.base = gdt_base, .limit = gdt_limit};

    CPUE_ASSERT(!m_cpu->load_segment_register(SegmentRegisterAlias::CS, SegmentSelector(1 << 3)).raised(), "exception while loading CS.");
    CPUE_ASSERT(!m_cpu->load_segment_register(SegmentRegisterAlias::DS, SegmentSelector(2 << 3)).raised(), "exception while loading DS.");
}


void UEFI::setup_idt(u64& top) {
    // This will set up a temporary IDT to use in long mode

    top = 0x5000;
    CPUE_ASSERT(m_cpu->mmu().physmem_size() >= top + 512 * 8, "setup_idt: Out of memory");
    u64 idt_base = top;

    // Just fill it with 510 entries
    for (int i = 0; i < 510; ++i) {
        CPUE_ASSERT(!m_cpu->mmu().mem_write64(top, 0x0).raised(), "exception while setting up IDT.");
        top += 0x8;
    }

    // IDT Pointer (2byte limit + 8byte linear base address)
    u64 idt_pointer = top;
    u16 idt_limit = top - idt_base - 1;
    CPUE_ASSERT(!m_cpu->mmu().mem_write16(top, idt_limit).raised(), "exception while setting up IDT.");
    top += 0x2;
    CPUE_ASSERT(!m_cpu->mmu().mem_write64(top, idt_base).raised(), "exception while setting up IDT.");
    top += 0x8;

    CPUE_ASSERT(!m_cpu->handle_CLI({}).raised(), "exception while setting up IDT");
    // TODO: maybe use actual lidt [idt_pointer] insn
    m_cpu->m_idtr = {.base = idt_base, .limit = idt_limit};
}

void UEFI::setup_stack(u64& top) {
    top = 0x6000 + 0xff0;
    CPUE_ASSERT(m_cpu->mmu().physmem_size() >= top, "setup_stack: Out of memory");

    CPUE_ASSERT(!m_cpu->load_segment_register(SegmentRegisterAlias::SS, SegmentSelector(2 << 3)).raised(), "exception while loading SS.");
    m_cpu->m_rsp_val = top;
}

void UEFI::reserve_scratch_space(u64& top) {
    top = 0x7000;
    top += PAGE_SIZE * 1;
    CPUE_ASSERT(m_cpu->mmu().physmem_size() >= top, "reserve_scratch_space: Out of memory");
}


}