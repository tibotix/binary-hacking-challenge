#include "mmu.h"
#include "checked_arithmetic.h"
#include "cpu.h"

namespace CPUE {


InterruptRaisedOr<PhysicalAddress> MMU::la_to_pa(LogicalAddress const& laddr, TranslationContext const& ctx) {
    // TODO: do alignment checking (only on data/stack accesses)
    StartLogicalAddressTranslation _(this, laddr);
    auto vaddr = MAY_HAVE_RAISED(la_to_va(laddr, ctx));
    return va_to_pa(vaddr, ctx);
}

InterruptRaisedOr<PhysicalAddress> MMU::va_to_pa(VirtualAddress const& vaddr, TranslationContext const& ctx) {
    // If paging is disabled, VirtualAddresses are treated as PhysicalAddresses (no paging translation is happening).
    if (!m_cpu->is_paging_enabled())
        return PhysicalAddress(vaddr.addr);

    StartVirtualAddressTranslation _(this, vaddr);
    // In IA-32e mode (if IA32_EFER.LMA = 1), linear addresses may undergo some pre-processing before being translated through paging.
    // Some of this pre-processing is done only if enabled by software, but some occurs unconditionally.
    // Specifically, linear addresses are subject to pre-processing in IA-32e mode as follows:
    // 1. Linear-address-space separation (LASS) (we don't implement that)
    // 2. Linear-address masking (LAM) (we don't implement that)
    // 3. Canonicality checking (we implement that)
    MAY_HAVE_RAISED(m_cpu->do_canonicality_check(vaddr));
    PTE pte = MAY_HAVE_RAISED(va_to_pte(vaddr, ctx));

    // the final physical address is computed as follows:
    // Bits 51:12 are from the PTE.
    // Bits 11:0 are from the original linear address.
    u64 paddr = (pte.c.paddr << 12) | bits(vaddr.addr, 11, 0);
    return paddr;
}

InterruptRaisedOr<PTE> MMU::va_to_pte(VirtualAddress const& vaddr, TranslationContext const& ctx) {
    m_cpu->assert_paging_enabled();

    // Query TLB for translation - we might be lucky
    if (std::optional<TLBEntry> entry; entry = m_tlb.lookup(vaddr), entry.has_value()) {
        PTE pte{{
            .present = 1,
            .rw = entry->rw,
            .user = entry->user,
            .pwt = 0,
            .pcd = 0,
            .accessed = 1,
            .dirty = entry->dirty,
            .pat = 0,
            .global = entry->global,
            .r = 0,
            .paddr = entry->paddr,
            .pkey = entry->pkey,
            .xd = entry->xd,
        }};
        // access checks still need to be performed
        MAY_HAVE_RAISED(check_page_structure((PageStructureEntry*)&pte, ctx));
        // if we pass checks, return
        return pte;
    }

    // Do page-table-walk
    auto result = MAY_HAVE_RAISED(page_table_walk(vaddr, ctx));

    if (ctx.op == OP_WRITE)
        result.pte->c.dirty = 1;

    // Cache entry for future lookups
    TLBEntry const entry = {
        .paddr = result.pte->c.paddr,
        .rw = (u64)(result.pml4e->c.rw & result.pdpte->c.rw & result.pde->c.rw & result.pte->c.rw),
        .user = (u64)(result.pml4e->c.user & result.pdpte->c.user & result.pde->c.user & result.pte->c.user),
        .xd = (u64)(result.pml4e->c.xd | result.pdpte->c.xd | result.pde->c.xd | result.pte->c.xd),
        .dirty = result.pte->c.dirty,
        .global = result.pte->c.global,
        .pkey = result.pte->c.pkey,
        .pcid = m_cpu->pcid(),
    };
    m_tlb.insert(vaddr, entry);

    return *result.pte;
}


InterruptRaisedOr<PTE*> MMU::va_to_pte_no_tlb(VirtualAddress const& vaddr, TranslationContext const& ctx) {
    auto result = MAY_HAVE_RAISED(page_table_walk(vaddr, ctx));
    if (ctx.op == OP_WRITE)
        result.pte->c.dirty = 1;
    return result.pte;
}

auto MMU::page_table_walk(VirtualAddress const& vaddr, TranslationContext const& ctx) -> InterruptRaisedOr<PageTableWalkResult> {
    auto data = std::make_tuple<MMU*, TranslationContext const*>(this, &ctx);
    auto result = page_table_walk(
        vaddr,
        [](PageStructureEntry* entry, void* d) -> bool {
            auto t = *static_cast<decltype(data)*>(d);
            return !std::get<0>(t)->check_page_structure(entry, *std::get<1>(t)).raised();
        },
        &data);
    if (result.aborted)
        return INTERRUPT_RAISED;
    return result;
};

auto MMU::page_table_walk(VirtualAddress const& vaddr, bool (*f)(PageStructureEntry*, void*), void* data) -> PageTableWalkResult {
    // See Section 5.5.4
    PageTableWalkResult r;

    // A 4-KByte naturally aligned PML4 table is located at the physical address specified in bits 51:12 of CR3
    // A PML4E is selected using the physical address defined as follows:
    // Bits 51:12 are from CR3 or the HLATP
    // Bits 11:3 are bits 47:39 of the linear address
    // Bits 2:0 are all 0
    {
        u64 pml4e_paddr = (m_cpu->cr3().c.pml4_base_paddr << 12) | (bits(vaddr.addr, 47, 39) << 3);
        TODO_NOFAIL("check endianness in paddr_ptr");
        r.pml4e = paddr_ptr<PML4E>(pml4e_paddr);
    }
    r.pml4e->c.accessed = 1;
    if (!f((PageStructureEntry*)(r.pml4e), data))
        goto abort_page_table_walk;

    // A 4-KByte naturally aligned page-directory-pointer table is located at the physical address specified in bits 51:12 of the PML4E
    // A PDPTE is selected using the physical address defined as follows:
    // Bits 51:12 are from the PML4E.
    // Bits 11:3 are bits 38:30 of the linear address.
    // Bits 2:0 are all 0
    {
        u64 pdpte_paddr = (r.pml4e->c.paddr << 12) | (bits(vaddr.addr, 38, 30) << 3);
        r.pdpte = paddr_ptr<PDPTE>(pdpte_paddr);
    }
    r.pdpte->c.accessed = 1;
    CPUE_ASSERT(r.pdpte->c.page_size == 0, "we don't support huge pages");
    if (!f((PageStructureEntry*)r.pdpte, data))
        goto abort_page_table_walk;

    // If the PDPTE’s PS flag is 0, a 4-KByte naturally aligned page directory is located at the physical address specified in bits 51:12 of the PDPTE
    // A PDE is selected using the physical address defined as follows:
    // Bits 51:12 are from the PDPTE.
    // Bits 11:3 are bits 29:21 of the linear address.
    // Bits 2:0 are all 0.
    {
        u64 pde_paddr = (r.pdpte->c.paddr << 12) | (bits(vaddr.addr, 29, 21) << 3);
        r.pde = paddr_ptr<PDE>(pde_paddr);
    }
    r.pde->c.accessed = 1;
    CPUE_ASSERT(r.pde->c.page_size == 0, "we don't support huge pages");
    if (!f((PageStructureEntry*)r.pde, data))
        goto abort_page_table_walk;

    // If the PDE’s PS flag is 0, a 4-KByte naturally aligned page table is located at the physical address specified in bits 51:12 of the PDE
    // A PTE is selected using the physical address defined as follows:
    // Bits 51:12 are from the PDE.
    // Bits 11:3 are bits 20:12 of the linear address.
    // Bits 2:0 are all 0
    {
        u64 pte_paddr = (r.pde->c.paddr << 12) | (bits(vaddr.addr, 20, 12) << 3);
        r.pte = paddr_ptr<PTE>(pte_paddr);
    }
    r.pte->c.accessed = 1;
    if (!f((PageStructureEntry*)r.pte, data))
        goto abort_page_table_walk;
    return r;

abort_page_table_walk:
    r.aborted = true;
    return r;
}


InterruptRaisedOr<void> MMU::check_page_structure_reserved_bits(HasReservedBits auto* entry, TranslationContext const& ctx) {
    if (entry->reserved_bits_ored() == 0)
        return {};
    ErrorCode const error_code = {.pf = {
                                      .present = 1,
                                      .wr = (u8)(ctx.op == OP_READ ? 0 : 1),
                                      .us = (u8)(m_cpu->cpm() == CPU::USER_MODE ? 1 : 0),
                                      .reserved = 1,
                                      .id = m_cpu->state() == CPU::State::STATE_FETCH_INSTRUCTION,
                                  }};
    return raise_page_fault(error_code);
}

InterruptRaisedOr<void> MMU::check_page_structure_present(PageStructureEntry* entry, TranslationContext const& ctx) {
    /**
     * If a paging-structure entry’s P flag (bit 0) is 0 or if the entry sets any reserved bit,
     * the entry is used neither to reference another paging-structure entry nor to map a page.
     * There is no translation for a linear address whose translation would use such a paging-structure entry;
     * a reference to such a linear address causes a page-fault exception.
     */
    if (entry->c.present == 0) {
        ErrorCode const error_code = {.pf = {
                                          .present = 0,
                                          .wr = (u8)(ctx.op == OP_READ ? 0 : 1),
                                          .us = (u8)(m_cpu->cpm() == CPU::USER_MODE ? 1 : 0),
                                          .reserved = 0,
                                          .id = m_cpu->state() == CPU::State::STATE_FETCH_INSTRUCTION,
                                      }};
        return raise_page_fault(error_code);
    }
    return {};
}

InterruptRaisedOr<void> MMU::check_page_structure_access_rights(PageStructureEntry* entry, TranslationContext const& ctx) {
    ErrorCode const error_code = {.pf = {
                                      .present = 1,
                                      .wr = (u8)(ctx.op == OP_READ ? 0 : 1),
                                      .us = (u8)(m_cpu->cpm() == CPU::USER_MODE ? 1 : 0),
                                      .reserved = 0,
                                      .id = m_cpu->state() == CPU::State::STATE_FETCH_INSTRUCTION,
                                  }};

    /**
     * Every access to a linear address is either a supervisor-mode access or a user-mode access. For all instruction
     * fetches and most data accesses, this distinction is determined by the current privilege level (CPL): accesses made
     * while CPL < 3 are supervisor-mode accesses, while accesses made while CPL = 3 are user-mode accesses.
     * Some operations implicitly access system data structures with linear addresses; the resulting accesses to those
     * data structures are supervisor-mode accesses regardless of CPL.
     * The following types of memory accesses are checked as if they are privilege-level 0 accesses,
     * regardless of the CPL at which the processor is currently operating:
     */
    // - Access to segment descriptors in the GDT, LDT, or IDT.
    auto cpm = m_cpu->cpm();
    bool implicit_access = false;
    if (ctx.intention == INTENTION_LOAD_DESCRIPTOR) {
        cpm = CPU::SUPERVISOR_MODE;
        implicit_access = true;
    }
    // - Access to an inner-privilege-level stack during an inter-privilege-level call or a call to an exception or interrupt
    //   handler, when a change of privilege level occurs.
    // - and accesses to the task-state segment (TSS) as part of a task switch or change of CPL.
    TODO_NOFAIL("Overrides to Page Protection");


    /**
     * Restricting Addressable Domain:
     * The segment privilege levels map to the page privilege levels as follows. If the processor is currently operating at
     * a CPL of 0, 1, or 2, it is in supervisor mode; if it is operating at a CPL of 3, it is in user mode. When the processor is
     * in supervisor mode, it can access all pages; when in user mode, it can access only user-level pages.
     */
    if (cpm == CPU::USER_MODE && !entry->c.user)
        return raise_page_fault(error_code);


    if (cpm == CPU::SUPERVISOR_MODE && entry->c.user) {
        // SMEP
        if (m_cpu->state() == CPU::State::STATE_FETCH_INSTRUCTION && m_cpu->cr4().c.SMEP == 1)
            return raise_page_fault(error_code);
        // SMAP
        if (m_cpu->cr4().c.SMAP == 1 && (m_cpu->m_rflags.c.AC == 0 || implicit_access))
            return raise_page_fault(error_code);
    }

    /**
     * Page Type:
     * The page-level protection mechanism recognizes two page types:
     *  - Read-only access (R/W flag is 0).
     *  - Read/write access (R/W flag is 1).
     * When the processor is in supervisor mode and the WP flag in register CR0 is clear (its state following reset initial-
     * ization), all pages are both readable and writable (write-protection is ignored).
     */
    // If CR0.WP = 1, read-only pages are not writable from any privilege level (useful for Copy-on-write)
    if (m_cpu->cr0().c.WP == 1 && ctx.op == OP_WRITE && entry->c.rw == 0)
        return raise_page_fault(error_code);
    // When the processor is in user mode, it can write only to user-mode pages that are read/write accessible.
    if (cpm == CPU::USER_MODE && ctx.op == OP_WRITE && entry->c.rw == 0)
        return raise_page_fault(error_code);

    /**
     * PAGE-LEVEL PROTECTION AND EXECUTE-DISABLE BIT
     * Instructions cannot be fetched from a memory page if IA32_EFER.NXE =1 and the execute-disable bit is set in any of
     * the paging-structure entries used to map the page.
     * An attempt to fetch instructions from a memory page with the execute-disable bit set causes a page-fault exception.
     */
    // TODO: maybe use ctx.intention == INTENTION_FETCH_INSTRUCTION
    if (m_cpu->m_efer.c.NXE == 1 && m_cpu->state() == CPU::State::STATE_FETCH_INSTRUCTION && entry->c.xd == 1)
        return raise_page_fault(error_code);

    return {};
}


InterruptRaisedOr<void> MMU::check_page_structure_protection_key(PageStructureEntry* entry, TranslationContext const& ctx) {
    // On instruction fetches PK is not checked
    if (m_cpu->state() != CPU::State::STATE_FETCH_INSTRUCTION)
        return {};
    // We currently do not implement protection key's
    return {};
}


_InterruptRaised MMU::raise_page_fault(VirtualAddress const& vaddr, ErrorCode const& error_code) {
    m_cpu->m_cr2_val = vaddr.addr;
    // In particular, a page-fault exception resulting from an attempt to use a linear address will invalidate any
    // TLB entries that are for a page number corresponding to that linear address and that are associated with the
    // current PCID.
    // These invalidations ensure that the page-fault exception will not
    // recur(if the faulting instruction is re-executed) if it would not be caused by the contents of the paging structures
    // in memory (and if, therefore, it resulted from cached entries that were not invalidated after the paging structures
    // were modified in memory).
    m_tlb.invalidate(vaddr);
    return m_cpu->raise_interrupt(Exceptions::PF(error_code));
}
_InterruptRaised MMU::raise_page_fault(ErrorCode const& error_code) {
    CPUE_ASSERT(m_currently_translating_vaddr.has_value(), "Trying to raise a page-fault while no VirtualAddress translation is in progress.");
    return raise_page_fault(m_currently_translating_vaddr.value(), error_code);
}



// See chapter 3.4 (page 3171)
InterruptRaisedOr<VirtualAddress> MMU::la_to_va(LogicalAddress const& laddr, TranslationContext const& ctx) {
    m_cpu->assert_in_long_mode();

    ErrorCode const error_code = [&]() -> ErrorCode {
        if (ctx.intention != TranslationIntention::INTENTION_LOAD_DESCRIPTOR)
            return ZERO_ERROR_CODE_NOEXT;
        return {.standard = {
                    .tbl = static_cast<u8>(laddr.segment_register.visible.segment_selector.c.table << 1),
                    .selector_index = laddr.segment_register.visible.segment_selector.c.index,
                }};
    }();

    // To translate a logical address into a linear address, the processor does the following:
    // 1.   Uses the offset in the segment selector to locate the segment descriptor for the segment in the GDT or LDT and
    //      reads it into the processor. (This step is needed only when a new segment selector is loaded into a segment
    //      register.)
    //      -> we already have the cached_descriptor in our shadow register
    ApplicationSegmentDescriptor const& descriptor = laddr.segment_register.hidden.cached_descriptor;
    if (descriptor.access.descriptor_type() == DescriptorType::CODE_SEGMENT) {
        CPUE_ASSERT(descriptor.l == 1, "Only 64-bit code segments are supported");
        CPUE_ASSERT(descriptor.l == 1 && descriptor.db == 1, "#GP");
    }

    // 2.   Examines the segment descriptor to check the access rights and range of the segment to ensure that the
    //      segment is accessible and that the offset is within the limits of the segment.
    //
    /**
     * Limit Checking (Chapter 6.3 or page 3249)
     * In 64-bit mode, the processor does not perform runtime limit checking on code or data segments.
     * However, the processor does check descriptor-table limits.
     * -> we do check descriptor-table limits in segment_selector_to_descriptor/interrupt_vector_to_descriptor,
     *    so no need to check it here.
     */

    /**
     * Type Checking (Chapter 6.5 or page 3250)
     * NOTE: 64-bit mode does not perform NULL-selector runtime checking
     * Certain segments can be used by instructions only in certain predefined ways, for example:
     *   — No instruction may write into an executable segment.
     *   — No instruction may write into a data segment if it is not writable.
     *   — No instruction may read an executable segment unless the readable flag is set
     */
    if (ctx.op == MemoryOp::OP_WRITE && descriptor.access.c.executable) {
        return m_cpu->raise_interrupt(Exceptions::GP(error_code));
    }
    if (ctx.op == MemoryOp::OP_WRITE && descriptor.access.descriptor_type() == DescriptorType::DATA_SEGMENT && !descriptor.access.c.wr) {
        return m_cpu->raise_interrupt(Exceptions::GP(error_code));
    }
    if (ctx.op == MemoryOp::OP_READ && descriptor.access.c.executable && !descriptor.access.c.wr) {
        return m_cpu->raise_interrupt(Exceptions::GP(error_code));
    }

    // 3.   Adds the base address of the segment from the segment descriptor to the offset to form a linear address.
    // Code segments continue to exist in 64-bit mode even though, for address calculations, the segment base is treated as zero.
    // -> we don't even allow setting the base of any ApplicationSegmentRegister, so it is always null.
    // TODO: check for FS,GS and use m_fsbase,m_gsbase

    return {descriptor.base() + laddr.offset};
}



InterruptRaisedOr<GDTLDTDescriptor> MMU::segment_selector_to_descriptor(SegmentSelector selector) {
    ErrorCode error_code = {.standard = {
                                .tbl = static_cast<u8>(selector.c.table << 1),
                                .selector_index = selector.c.index,
                            }};
    auto [base, limit] = m_cpu->descriptor_table_of_selector(selector);
    return get_descriptor_from_descriptor_table<GDTLDTDescriptor, 8>(base, limit, selector.c.index, error_code);
}

InterruptRaisedOr<IDTDescriptor> MMU::interrupt_vector_to_descriptor(InterruptVector vector) {
    ErrorCode error_code = {.standard = {.tbl = 1, .selector_index = vector}};
    auto& idtr = m_cpu->m_idtr;
    // In 64-bit mode, the IDT index is formed by scaling the interrupt vector by 16.
    return get_descriptor_from_descriptor_table<IDTDescriptor, 16>(idtr.base, idtr.limit, vector, error_code);
}


template<typename D, u16 index_scale>
InterruptRaisedOr<D> MMU::get_descriptor_from_descriptor_table(VirtualAddress const& table_base, u16 table_limit, u16 descriptor_index, ErrorCode error_code) {
    if (table_limit < CPUE_checked_umul<u16, u16>((descriptor_index + 1), index_scale)) {
        return m_cpu->raise_interrupt(Exceptions::GP(error_code));
    }
    TranslationContext ctx = {
        .op = MemoryOp::OP_READ,
        .width = ByteWidth::WIDTH_QWORD,
        .intention = TranslationIntention::INTENTION_LOAD_DESCRIPTOR,
    };
    TODO_NOFAIL("check endianness (get_descriptor_from_descriptor_table)");
    auto* descriptor1 = paddr_ptr<Descriptor>(MAY_HAVE_RAISED(va_to_pa(table_base + (descriptor_index * index_scale), ctx)));
    if (descriptor1->access.descriptor_type() == DescriptorType::RESERVED)
        return m_cpu->raise_interrupt(Exceptions::GP(error_code));
    // If the P flag is set to 0, a not present (#NP) exception is generated when a program attempts to access the descriptor. (Can be used by OS to track access counts)
    if (!descriptor1->access.c.present)
        return m_cpu->raise_interrupt(Exceptions::NP(error_code));
    if (!descriptor1->access.is_expanded_descriptor()) {
        return *(D*)descriptor1;
    }
    // check if table has enough size for expanded descriptor
    if (table_limit < CPUE_checked_uadd<u16, u16>((descriptor_index + 1) * index_scale, sizeof(Descriptor)))
        return m_cpu->raise_interrupt(Exceptions::GP(error_code));
    // check if we have access to the next 64 bit
    auto* descriptor2 = paddr_ptr<Descriptor>(MAY_HAVE_RAISED(va_to_pa(table_base + ((descriptor_index + 1) * index_scale) + sizeof(Descriptor), ctx)));
    // this should be always 0
    if (descriptor1->access.type_value() != 0)
        return m_cpu->raise_interrupt(Exceptions::GP(error_code));
    D expanded_descriptor{};
    memcpy(&expanded_descriptor, descriptor1, sizeof(Descriptor));
    memcpy(&expanded_descriptor + 8, descriptor2, sizeof(Descriptor));
    return expanded_descriptor;
}

template<unsigned_integral T>
InterruptRaisedOr<T> MMU::mem_read(LogicalAddress const& laddr, TranslationIntention intention) {
    TranslationContext ctx = {
        .op = MemoryOp::OP_READ,
        .width = get_byte_width<T>(),
        .intention = intention,
    };
    auto const paddr = MAY_HAVE_RAISED(la_to_pa(laddr, ctx));
    // TODO: make return type BigEndian<T> or LittleEndian<T>
    if (auto opt = MAY_HAVE_RAISED(m_mmio.try_mmio_read<T>(paddr)); opt.has_value())
        return opt->value;
    return *paddr_ptr<T>(paddr);
}

template<unsigned_integral T>
InterruptRaisedOr<T> MMU::mem_read(VirtualAddress const& vaddr, TranslationIntention intention) {
    TranslationContext ctx = {
        .op = MemoryOp::OP_READ,
        .width = get_byte_width<T>(),
        .intention = intention,
    };
    auto const paddr = MAY_HAVE_RAISED(va_to_pa(vaddr, ctx));
    // TODO: make return type BigEndian<T> or LittleEndian<T>
    if (auto opt = MAY_HAVE_RAISED(m_mmio.try_mmio_read<T>(paddr)); opt.has_value())
        return opt->value;
    return *paddr_ptr<T>(paddr);
}
InterruptRaisedOr<u64> MMU::mem_read64(LogicalAddress const& laddr, TranslationIntention intention) {
    return mem_read<u64>(laddr, intention);
}
InterruptRaisedOr<u64> MMU::mem_read64(VirtualAddress const& vaddr, TranslationIntention intention) {
    return mem_read<u64>(vaddr, intention);
}
InterruptRaisedOr<u32> MMU::mem_read32(LogicalAddress const& laddr, TranslationIntention intention) {
    return mem_read<u32>(laddr, intention);
}
InterruptRaisedOr<u32> MMU::mem_read32(VirtualAddress const& vaddr, TranslationIntention intention) {
    return mem_read<u32>(vaddr, intention);
}
InterruptRaisedOr<u16> MMU::mem_read16(LogicalAddress const& laddr, TranslationIntention intention) {
    return mem_read<u16>(laddr, intention);
}
InterruptRaisedOr<u16> MMU::mem_read16(VirtualAddress const& vaddr, TranslationIntention intention) {
    return mem_read<u16>(vaddr, intention);
}
InterruptRaisedOr<u8> MMU::mem_read8(LogicalAddress const& laddr, TranslationIntention intention) {
    return mem_read<u8>(laddr, intention);
}
InterruptRaisedOr<u8> MMU::mem_read8(VirtualAddress const& vaddr, TranslationIntention intention) {
    return mem_read<u8>(vaddr, intention);
}

template<unsigned_integral T>
InterruptRaisedOr<void> MMU::mem_write(LogicalAddress const& laddr, T const& value, TranslationIntention intention) {
    TranslationContext ctx = {
        .op = MemoryOp::OP_WRITE,
        .width = get_byte_width<T>(),
        .intention = intention,
    };
    auto const paddr = MAY_HAVE_RAISED(la_to_pa(laddr, ctx));
    if (MAY_HAVE_RAISED(m_mmio.try_mmio_write<T>(paddr, value)))
        return {};
    *paddr_ptr<T>(paddr) = value;
    return {};
}

template<unsigned_integral T>
InterruptRaisedOr<void> MMU::mem_write(VirtualAddress const& vaddr, T const& value, TranslationIntention intention) {
    TranslationContext ctx = {
        .op = MemoryOp::OP_WRITE,
        .width = get_byte_width<T>(),
        .intention = intention,
    };
    auto const paddr = MAY_HAVE_RAISED(va_to_pa(vaddr, ctx));
    if (MAY_HAVE_RAISED(m_mmio.try_mmio_write<T>(paddr, value)))
        return {};
    *paddr_ptr<T>(paddr) = value;
    return {};
}
InterruptRaisedOr<void> MMU::mem_write64(LogicalAddress const& laddr, u64 value, TranslationIntention intention) {
    return mem_write<u64>(laddr, value, intention);
}
InterruptRaisedOr<void> MMU::mem_write64(VirtualAddress const& vaddr, u64 value, TranslationIntention intention) {
    return mem_write<u64>(vaddr, value, intention);
}
InterruptRaisedOr<void> MMU::mem_write32(LogicalAddress const& laddr, u32 value, TranslationIntention intention) {
    return mem_write<u32>(laddr, value, intention);
}
InterruptRaisedOr<void> MMU::mem_write32(VirtualAddress const& vaddr, u32 value, TranslationIntention intention) {
    return mem_write<u32>(vaddr, value, intention);
}
InterruptRaisedOr<void> MMU::mem_write16(LogicalAddress const& laddr, u16 value, TranslationIntention intention) {
    return mem_write<u16>(laddr, value, intention);
}
InterruptRaisedOr<void> MMU::mem_write16(VirtualAddress const& vaddr, u16 value, TranslationIntention intention) {
    return mem_write<u16>(vaddr, value, intention);
}
InterruptRaisedOr<void> MMU::mem_write8(LogicalAddress const& laddr, u8 value, TranslationIntention intention) {
    return mem_write<u8>(laddr, value, intention);
}
InterruptRaisedOr<void> MMU::mem_write8(VirtualAddress const& vaddr, u8 value, TranslationIntention intention) {
    return mem_write<u8>(vaddr, value, intention);
}




}