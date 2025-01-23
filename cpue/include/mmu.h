#pragma once

#include <cstdlib>
#include <cstdint>
#include <cmath>
#include <sys/mman.h>
#include <iostream>

#include "common.h"
#include "tlb.h"
#include "paging.h"
#include "address.h"
#include "controllers/pic.h"
#include "descriptors/gdtldt_descriptors.h"
#include "descriptors/idt_descriptors.h"
#include "forward.h"
#include "mmio.h"
#include "endianness.h"


namespace CPUE {

enum TranslationIntention {
    INTENTION_FETCH_INSTRUCTION,
    INTENTION_HANDLE_INSTRUCTION,
    INTENTION_LOAD_DESCRIPTOR,
    INTENTION_LOAD_TSS,
    INTENTION_UNKNOWN,
};
struct TranslationContext {
    MemoryOp op;
    ByteWidth width;
    TranslationIntention intention;
    void* data = NULL;
};

class MMU {
public:
    friend Loader;
    MMU(CPU* cpu, size_t available_pages)
        : m_tlb(TLB{cpu}), m_cpu(cpu), m_mmio(MMIO{}), m_physmem_size(available_pages * PAGE_SIZE), m_available_pages(available_pages), m_physmem(NULL) {
        CPUE_ASSERT(cpu != NULL, "cpu is NULL");
        allocate_physmem();
    }
    MMU(MMU const&) = delete;

    MMIO& mmio() { return m_mmio; }
    TLB& tlb() { return m_tlb; }
    size_t physmem_size() const { return m_physmem_size; }
    size_t available_pages() const { return m_available_pages; }

    [[nodiscard]] InterruptRaisedOr<PhysicalAddress> la_to_pa(LogicalAddress const& laddr, TranslationContext const& ctx);

    [[nodiscard]] InterruptRaisedOr<GDTLDTDescriptor> segment_selector_to_descriptor(SegmentSelector selector);
    [[nodiscard]] InterruptRaisedOr<IDTDescriptor> interrupt_vector_to_descriptor(InterruptVector vector);


    [[nodiscard]] InterruptRaisedOr<u64> mem_read64(LogicalAddress const& laddr, TranslationIntention intention = TranslationIntention::INTENTION_UNKNOWN);
    [[nodiscard]] InterruptRaisedOr<u64> mem_read64(VirtualAddress const& vaddr, TranslationIntention intention = TranslationIntention::INTENTION_UNKNOWN);
    [[nodiscard]] InterruptRaisedOr<u32> mem_read32(LogicalAddress const& laddr, TranslationIntention intention = TranslationIntention::INTENTION_UNKNOWN);
    [[nodiscard]] InterruptRaisedOr<u32> mem_read32(VirtualAddress const& vaddr, TranslationIntention intention = TranslationIntention::INTENTION_UNKNOWN);
    [[nodiscard]] InterruptRaisedOr<u16> mem_read16(LogicalAddress const& laddr, TranslationIntention intention = TranslationIntention::INTENTION_UNKNOWN);
    [[nodiscard]] InterruptRaisedOr<u16> mem_read16(VirtualAddress const& vaddr, TranslationIntention intention = TranslationIntention::INTENTION_UNKNOWN);
    [[nodiscard]] InterruptRaisedOr<u8> mem_read8(LogicalAddress const& laddr, TranslationIntention intention = TranslationIntention::INTENTION_UNKNOWN);
    [[nodiscard]] InterruptRaisedOr<u8> mem_read8(VirtualAddress const& vaddr, TranslationIntention intention = TranslationIntention::INTENTION_UNKNOWN);

    [[nodiscard]] InterruptRaisedOr<void> mem_write64(LogicalAddress const& laddr, u64 value, TranslationIntention intention = TranslationIntention::INTENTION_UNKNOWN);
    [[nodiscard]] InterruptRaisedOr<void> mem_write64(VirtualAddress const& vaddr, u64 value, TranslationIntention intention = TranslationIntention::INTENTION_UNKNOWN);
    [[nodiscard]] InterruptRaisedOr<void> mem_write32(LogicalAddress const& laddr, u32 value, TranslationIntention intention = TranslationIntention::INTENTION_UNKNOWN);
    [[nodiscard]] InterruptRaisedOr<void> mem_write32(VirtualAddress const& vaddr, u32 value, TranslationIntention intention = TranslationIntention::INTENTION_UNKNOWN);
    [[nodiscard]] InterruptRaisedOr<void> mem_write16(LogicalAddress const& laddr, u16 value, TranslationIntention intention = TranslationIntention::INTENTION_UNKNOWN);
    [[nodiscard]] InterruptRaisedOr<void> mem_write16(VirtualAddress const& vaddr, u16 value, TranslationIntention intention = TranslationIntention::INTENTION_UNKNOWN);
    [[nodiscard]] InterruptRaisedOr<void> mem_write8(LogicalAddress const& laddr, u8 value, TranslationIntention intention = TranslationIntention::INTENTION_UNKNOWN);
    [[nodiscard]] InterruptRaisedOr<void> mem_write8(VirtualAddress const& vaddr, u8 value, TranslationIntention intention = TranslationIntention::INTENTION_UNKNOWN);

    template<unsigned_integral T>
    [[nodiscard]] InterruptRaisedOr<T> mem_read(LogicalAddress const& laddr, TranslationIntention intention = TranslationIntention::INTENTION_UNKNOWN) {
        TranslationContext ctx = {
            .op = MemoryOp::OP_READ,
            .width = get_byte_width<T>(),
            .intention = intention,
        };
        auto const vaddr = MAY_HAVE_RAISED(la_to_va(laddr, ctx));
        return mem_read<T>(vaddr, intention);
    }
    template<unsigned_integral T>
    [[nodiscard]] InterruptRaisedOr<T> mem_read(VirtualAddress const& vaddr, TranslationIntention intention = TranslationIntention::INTENTION_UNKNOWN) {
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
    template<unsigned_integral T>
    [[nodiscard]] InterruptRaisedOr<void> mem_write(LogicalAddress const& laddr, T const& value, TranslationIntention intention = TranslationIntention::INTENTION_UNKNOWN) {
        TranslationContext ctx = {
            .op = MemoryOp::OP_WRITE,
            .width = get_byte_width<T>(),
            .intention = intention,
        };
        auto const vaddr = MAY_HAVE_RAISED(la_to_va(laddr, ctx));
        return mem_write<T>(vaddr, value, intention);
    }
    template<unsigned_integral T>
    [[nodiscard]] InterruptRaisedOr<void> mem_write(VirtualAddress const& vaddr, T const& value, TranslationIntention intention = TranslationIntention::INTENTION_UNKNOWN) {
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

private:
    [[nodiscard]] InterruptRaisedOr<PhysicalAddress> va_to_pa(VirtualAddress const& vaddr, TranslationContext const& ctx);
    [[nodiscard]] InterruptRaisedOr<VirtualAddress> la_to_va(LogicalAddress const& laddr, TranslationContext const& ctx);
    [[nodiscard]] InterruptRaisedOr<PTE> va_to_pte(VirtualAddress const& vaddr, TranslationContext const& ctx);
    [[nodiscard]] InterruptRaisedOr<PTE*> va_to_pte_no_tlb(VirtualAddress const& vaddr, TranslationContext const& ctx);
    struct PageTableWalkResult {
        PageTableWalkResult() : pml4e(nullptr), pdpte(nullptr), pde(nullptr), pte(nullptr), aborted(false) {}
        PML4E* pml4e;
        PDPTE* pdpte;
        PDE* pde;
        PTE* pte;
        bool aborted;
    };
    [[nodiscard]] InterruptRaisedOr<PageTableWalkResult> page_table_walk(VirtualAddress const& vaddr, TranslationContext const& ctx);
    // TODO: maybe rename this to better capture the "generality" intention of this page_table_walk
    [[nodiscard]] PageTableWalkResult page_table_walk(VirtualAddress const& vaddr, bool (*f)(PageStructureEntry*, void*), void* data = nullptr);
    [[nodiscard]] InterruptRaisedOr<void> check_page_structure(PageStructureEntry* entry, TranslationContext const& ctx) {
        MAY_HAVE_RAISED(check_page_structure_reserved_bits(entry, ctx));
        MAY_HAVE_RAISED(check_page_structure_present(entry, ctx));
        MAY_HAVE_RAISED(check_page_structure_access_rights(entry, ctx));
        MAY_HAVE_RAISED(check_page_structure_protection_key(entry, ctx));
        return {};
    }
    [[nodiscard]] InterruptRaisedOr<void> check_page_structure_reserved_bits(HasReservedBits auto* entry, TranslationContext const& ctx);
    [[nodiscard]] InterruptRaisedOr<void> check_page_structure_present(PageStructureEntry* entry, TranslationContext const& ctx);
    [[nodiscard]] InterruptRaisedOr<void> check_page_structure_access_rights(PageStructureEntry* entry, TranslationContext const& ctx);
    [[nodiscard]] InterruptRaisedOr<void> check_page_structure_protection_key(PageStructureEntry* entry, TranslationContext const& ctx);
    _InterruptRaised raise_page_fault(VirtualAddress const& vaddr, ErrorCode const& error_code);
    _InterruptRaised raise_page_fault(ErrorCode const& error_code);

    template<typename D, u16 index_scale>
    [[nodiscard]] InterruptRaisedOr<D> get_descriptor_from_descriptor_table(VirtualAddress const& table_base, u16 table_limit, u16 descriptor_index, ErrorCode error_code);

    template<typename T = u8>
    T* paddr_ptr(PhysicalAddress const& paddr) {
        CPUE_ASSERT(m_physmem != NULL, "m_physmem == NULL");
        return (T*)(m_physmem + paddr.addr);
    }

    void allocate_physmem() {
        m_physmem = (u8*)mmap(NULL, m_physmem_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (m_physmem == (u8*)-1) {
            fail("mmap");
        }
    }

private:
    class StartVirtualAddressTranslation {
    public:
        StartVirtualAddressTranslation(MMU* instance, VirtualAddress const& vaddr) : m_instance(instance) { m_instance->m_currently_translating_vaddr = vaddr; }
        ~StartVirtualAddressTranslation() { m_instance->m_currently_translating_vaddr.reset(); }

    private:
        MMU* m_instance;
    };
    friend class StartVirtualAddressTranslation;
    std::optional<VirtualAddress> m_currently_translating_vaddr;

    class StartLogicalAddressTranslation {
    public:
        StartLogicalAddressTranslation(MMU* instance, LogicalAddress const& laddr) : m_instance(instance) { m_instance->m_currently_translating_laddr = laddr; }
        ~StartLogicalAddressTranslation() { m_instance->m_currently_translating_laddr.reset(); }

    private:
        MMU* m_instance;
    };
    friend class StartLogicalAddressTranslation;
    std::optional<LogicalAddress> m_currently_translating_laddr;

private:
    TLB m_tlb;
    CPU* m_cpu;
    MMIO m_mmio;
    size_t m_physmem_size;
    size_t m_available_pages;
    u8* m_physmem;
};

}