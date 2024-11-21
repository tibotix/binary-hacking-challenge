
#include "mmu.h"
#include "descriptors.h"

#include "cpu.h"

namespace CPUE {


InterruptRaisedOr<PhysicalAddress> MMU::la_to_pa(LogicalAddress const& laddr, ByteWidth width) {
    StartLogicalAddressTranslation(this, laddr);
    auto vaddr = MAY_HAVE_RAISED(la_to_va(laddr, width));
    return va_to_pa(vaddr, width);
}

InterruptRaisedOr<PhysicalAddress> MMU::va_to_pa(VirtualAddress const& vaddr, ByteWidth width) {
    StartVirtualAddressTranslation(this, vaddr);
    // In IA-32e mode (if IA32_EFER.LMA = 1), linear addresses may undergo some pre-processing before being translated through paging.
    // Some of this pre-processing is done only if enabled by software, but some occurs unconditionally.
    // Specifically, linear addresses are subject to pre-processing in IA-32e mode as follows:
    // 1. Linear-address-space separation (LASS) (we don't implement that)
    // 2. Linear-address masking (LAM) (we don't implement that)
    // 3. Canonicality checking (we implement that)
    MAY_HAVE_RAISED(m_cpu->do_canonicality_check(vaddr));
    PTE* pte = MAY_HAVE_RAISED(va_to_pte(vaddr));

    // the final physical address is computed as follows:
    // Bits 51:12 are from the PTE.
    // Bits 11:0 are from the original linear address.
    u64 paddr = pte->paddr + bits(vaddr.addr, 11, 0);
    return paddr;
}

InterruptRaisedOr<PTE*> MMU::va_to_pte(VirtualAddress const& vaddr) {
    m_cpu->assert_paging_enabled();
    TODO_NOFAIL("Query TLB for translation");

    // See Section 5.5.4

    // A 4-KByte naturally aligned PML4 table is located at the physical address specified in bits 51:12 of CR3
    // A PML4E is selected using the physical address defined as follows:
    // Bits 51:12 are from CR3 or the HLATP
    // Bits 11:3 are bits 47:39 of the linear address
    // Bits 2:0 are all 0
    u64 pml4e_paddr = m_cpu->m_cr3.pml4_base_paddr + bits(vaddr.addr, 47, 39);
    PML4E* pml4e = paddr_ptr<PML4E>(pml4e_paddr);
    MAY_HAVE_RAISED(check_page_structure(pml4e));

    // A 4-KByte naturally aligned page-directory-pointer table is located at the physical address specified in bits 51:12 of the PML4E
    // A PDPTE is selected using the physical address defined as follows:
    // Bits 51:12 are from the PML4E.
    // Bits 11:3 are bits 38:30 of the linear address.
    // Bits 2:0 are all 0
    u64 pdpte_paddr = pml4e->paddr + bits(vaddr.addr, 38, 30);
    PDPTE* pdpte = paddr_ptr<PDPTE>(pdpte_paddr);
    CPUE_ASSERT(pdpte->page_size = 0, "we don't support huge pages");
    MAY_HAVE_RAISED(check_page_structure(pdpte));

    // If the PDPTE’s PS flag is 0, a 4-KByte naturally aligned page directory is located at the physical address specified in bits 51:12 of the PDPTE
    // A PDE is selected using the physical address defined as follows:
    // Bits 51:12 are from the PDPTE.
    // Bits 11:3 are bits 29:21 of the linear address.
    // Bits 2:0 are all 0.
    u64 pde_paddr = pdpte->paddr + bits(vaddr.addr, 29, 21);
    PDE* pde = paddr_ptr<PDE>(pde_paddr);
    CPUE_ASSERT(pde->page_size = 0, "we don't support huge pages");
    MAY_HAVE_RAISED(check_page_structure(pde));

    // If the PDE’s PS flag is 0, a 4-KByte naturally aligned page table is located at the physical address specified in bits 51:12 of the PDE
    // A PTE is selected using the physical address defined as follows:
    // Bits 51:12 are from the PDE.
    // Bits 11:3 are bits 20:12 of the linear address.
    // Bits 2:0 are all 0
    u64 pte_paddr = pde->paddr + bits(vaddr.addr, 20, 12);
    PTE* pte = paddr_ptr<PTE>(pte_paddr);
    MAY_HAVE_RAISED(check_page_structure(pte));

    return pte;
}


InterruptRaisedOr<void> MMU::check_page_structure_present(PageStructureEntry auto* entry) {
    /**
     * If a paging-structure entry’s P flag (bit 0) is 0 or if the entry sets any reserved bit,
     * the entry is used neither to reference another paging-structure entry nor to map a page.
     * There is no translation for a linear address whose translation would use such a paging-structure entry;
     * a reference to such a linear address causes a page-fault exception.
     */
    if (entry->present == 0) {
        m_cpu->pic().deliver_interrupt(Exceptions::PF);
        // TODO: maybe use m_cpu->cr2() = ...
        m_cpu->m_cr2 = m_currently_translating_vaddr.value();
        TODO_NOFAIL("set error code accordingly");
        return INTERRUPT_RAISED;
    }
    return {};
}

InterruptRaisedOr<void> MMU::check_page_structure_access_rights(PageStructureEntry auto* entry) {
    TODO_NOFAIL("Check access rights");
    return {};
}


// See chapter 3.4 (page 3171)
InterruptRaisedOr<VirtualAddress> MMU::la_to_va(LogicalAddress const& laddr, ByteWidth width) {
    m_cpu->assert_in_long_mode();

    // To translate a logical address into a linear address, the processor does the following:

    // 1.   Uses the offset in the segment selector to locate the segment descriptor for the segment in the GDT or LDT and
    //      reads it into the processor. (This step is needed only when a new segment selector is loaded into a segment
    //      register.)
    // TODO: only do this when a new segment selector is loaded into a segment register
    // TODO: then also do type checking (Chapter 6.3 or page 3250)
    if (m_cpu->m_gdtr.limit < (laddr.segment_selector.index + 1) * sizeof(SegmentDescriptor)) {
        TODO_NOFAIL("which exception to deliver here?");
        return m_cpu->pic().deliver_interrupt(Exceptions::GP);
    }
    auto gdt_paddr = MAY_HAVE_RAISED(va_to_pa(m_cpu->m_gdtr.base));
    SegmentDescriptor* descriptor = (SegmentDescriptor*)(paddr_ptr(gdt_paddr) + laddr.segment_selector.index * 8);

    if (descriptor->segment_type() == SegmentType::CODE) {
        CPUE_ASSERT(descriptor->l == 1, "Only 64-bit code segments are supported");
        CPUE_ASSERT(descriptor->l == 1 && descriptor->db == 1, "#GP");
    }

    // 2.   Examines the segment descriptor to check the access rights and range of the segment to ensure that the
    //      segment is accessible and that the offset is within the limits of the segment.
    //
    /**
     * Limit Checking (Chapter 6.3 or page 3249)
     * In 64-bit mode, the processor does not perform runtime limit checking on code or data segments.
     * However, the processor does check descriptor-table limits.
     */
    if (descriptor->segment_type() == SegmentType::SYSTEM) {
        u64 effective_limit = descriptor->limit();
        if (descriptor->g == 1) {
            effective_limit *= 4_kb;
        }
        if (laddr.offset > effective_limit - (width - 1)) {
            TODO_NOFAIL("Determine if descriptor is SS (Stack segment) and deliver SF if it's the case");
            return m_cpu->pic().deliver_interrupt(Exceptions::GP);
        }
    }
    /**
     * Type Checking (Chapter 6.5 orpage 3250)
     * When instructions access segments whose descriptors are already loaded into segment registers —
     * Certain segments can be used by instructions only in certain predefined ways, for example:
     *   — No instruction may write into an executable segment.
     *   — No instruction may write into a data segment if it is not writable.
     *   — No instruction may read an executable segment unless the readable flag is set
     */
    // TODO: maybe pass OperationType along (Read/Write) or it is checked at the insn handling level
    TODO_NOFAIL("Type Checking");



    TODO_NOFAIL("Access Rights Checking");


    // Code segments continue to exist in 64-bit mode even though, for address calculations, the segment base is treated as zero
    u64 base = [&]() -> u64 {
        if (descriptor->segment_type() == SegmentType::CODE)
            return 0;
        return descriptor->base();
    }();

    // 3.   Adds the base address of the segment from the segment descriptor to the offset to form a linear address.
    return {base + laddr.offset};
}

}