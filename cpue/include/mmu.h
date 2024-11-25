#pragma once

#include <cstdlib>
#include <cstdint>
#include <sys/mman.h>
#include <iostream>

#include "common.h"
#include "tlb.h"
#include "paging.h"
#include "address.h"
#include "pic.h"
#include "descriptors/gdtldt_descriptors.h"
#include "descriptors/idt_descriptors.h"
#include "forward.h"


namespace CPUE {

enum TranslationIntention {
    INTENTION_FETCH_INSTRUCTION,
    INTENTION_HANDLE_INSTRUCTION,
    INTENTION_LOAD_SEGMENT,
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
    friend class CPU;
    friend class Disassembler;
    MMU(CPU* cpu, size_t available_pages) : m_cpu(cpu), m_tlb(TLB{cpu}), m_physmem(NULL), m_physmem_size(available_pages * PAGE_SIZE) { CPUE_ASSERT(cpu != NULL, "cpu != NULL"); }
    MMU(MMU const&) = delete;

    void init() {
        m_physmem = (u8*)mmap(NULL, m_physmem_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS, -1, 0);
        if (m_physmem == (u8*)-1) {
            fail("mmap");
        }
    }

public:
    bool mem_map();
    InterruptRaisedOr<PhysicalAddress> la_to_pa(LogicalAddress const& laddr, TranslationContext const& ctx);

    InterruptRaisedOr<GDTLDTDescriptor> segment_selector_to_descriptor(SegmentSelector selector);
    InterruptRaisedOr<IDTDescriptor> interrupt_vector_to_descriptor(InterruptVector vector);

    u32 mem_read32(LogicalAddress const& laddr);
    void mem_write32(LogicalAddress const& laddr, u32 value);
    u32 mem_read(LogicalAddress const& laddr, size_t width);

private:
    InterruptRaisedOr<PhysicalAddress> va_to_pa(VirtualAddress const& vaddr, TranslationContext const& ctx);
    InterruptRaisedOr<VirtualAddress> la_to_va(LogicalAddress const& laddr, TranslationContext const& ctx);
    InterruptRaisedOr<PTE> va_to_pte(VirtualAddress const& vaddr, TranslationContext const& ctx);
    InterruptRaisedOr<void> check_page_structure(PageStructureEntry auto* entry, TranslationContext const& ctx) {
        MAY_HAVE_RAISED(check_page_structure_reserved_bits(entry, ctx));
        MAY_HAVE_RAISED(check_page_structure_present(entry, ctx));
        MAY_HAVE_RAISED(check_page_structure_access_rights(entry, ctx));
        MAY_HAVE_RAISED(check_page_structure_protection_key(entry, ctx));
        return {};
    }
    InterruptRaisedOr<void> check_page_structure_reserved_bits(HasReservedBits auto* entry, TranslationContext const& ctx);
    InterruptRaisedOr<void> check_page_structure_present(PageStructureEntry auto* entry, TranslationContext const& ctx);
    InterruptRaisedOr<void> check_page_structure_access_rights(PageStructureEntry auto* entry, TranslationContext const& ctx);
    InterruptRaisedOr<void> check_page_structure_protection_key(PageStructureEntry auto* entry, TranslationContext const& ctx);
    _InterruptRaised raise_page_fault(VirtualAddress const& vaddr, ErrorCode const& error_code);
    _InterruptRaised raise_page_fault(ErrorCode const& error_code);
    template<typename T = u8>
    T* paddr_ptr(PhysicalAddress const& paddr) {
        CPUE_ASSERT(m_physmem != NULL, "m_physmem == NULL");
        return (T*)(m_physmem + paddr.addr);
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
    size_t m_physmem_size;
    u8* m_physmem;
};

}