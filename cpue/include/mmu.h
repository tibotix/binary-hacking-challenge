#pragma once

#include <cstdlib>
#include <stdint.h>
#include <sys/mman.h>
#include <iostream>
#include <concepts>

#include "common.h"
#include "tlb.h"
#include "address.h"
#include "pic.h"
#include "forward.h"


namespace CPUE {

// We only support one fixed PAGE_SIZE. We don't support huge pages such as 2MB Pages (referenced directly from PDE).
constexpr size_t PAGE_SIZE = 4_kb;
constexpr u8 VIRTUAL_ADDR_BITS = 48; // 4-Level paging enables up to 48bit VirtualAddress Space
constexpr u64 VIRTUAL_ADDR_MASK = ((u64)1 << VIRTUAL_ADDR_BITS) - 1;



template<typename T>
concept PageStructureEntry = requires(T entry) {
    { entry.present } -> std::convertible_to<u8>;
    { entry.rw } -> std::convertible_to<u8>;
    { entry.user } -> std::convertible_to<u8>;
    { entry.pwt } -> std::convertible_to<u8>;
    { entry.pcd } -> std::convertible_to<u8>;
    { entry.accessed } -> std::convertible_to<u8>;
    { entry.r } -> std::convertible_to<u8>;
    { entry.paddr } -> std::convertible_to<u8>;
    { entry.xd } -> std::convertible_to<u8>;
};
template<typename T>
concept ParentPageStructureEntry = PageStructureEntry<T> && requires(T entry) {
    { entry.page_size } -> std::convertible_to<u8>;
};


// Page-Map-Level-4 size: 512 entries
// Page-Map-Level-4 Entry
struct PML4E {
    // size: 64 bits
    u64 present : 1; // Present; must be 1 to reference a page-directory-pointer table
    u8 rw : 1; // Read/write; if 0, writes may not be allowed to the 512-GByte region controlled by this entry
    u8 user : 1; // User/supervisor; if 0, user-mode accesses are not allowed to the 512-GByte region controlled by this entry
    u8 pwt : 1; // Page-level write-through; indirectly determines the memory type used to access the page-directory-pointer table referenced by this entry
    u8 pcd : 1; // Page-level cache disable; indirectly determines the memory type used to access the page-directory-pointer table referenced by this entry
    u8 accessed : 1; // Accessed; indicates whether this entry has been used for linear-address translation
    u8 : 1; // Ignored
    u8 page_size : 1 = 0; // Reserved (must be 0)
    u8 : 3; // Ignored
    u8 r : 1; // For ordinary paging, ignored; for HLAT paging, restart (if 1, linear-address translation is restarted with ordinary paging)
    u64 paddr : VIRTUAL_ADDR_BITS - 13; // Physical address of 4-KByte aligned page-directory-pointer table referenced by this entry
    u64 : 51 - VIRTUAL_ADDR_BITS; // Reserved (must be 0)
    u16 : 11; // Ignored
    u8 xd : 1; // If IA32_EFER.NXE = 1, execute-disable (if 1, instruction fetches are not allowed from the 512-GByte region controlled by this entry); otherwise, reserved (must be 0)
};

// Page-Directory-Pointer-Table size: 512 entries
// Page-Directory-Pointer-Table Entry
struct PDPTE {
    // size: 64 bits
    u64 present : 1; // Present; must be 1 to reference a page directory
    u8 rw : 1; // Read/write; if 0, writes may not be allowed to the 1-GByte region controlled by this entry
    u8 user : 1; // User/supervisor; if 0, user-mode accesses are not allowed to the 1-GByte region controlled by this entry
    u8 pwt : 1; // Page-level write-through; indirectly determines the memory type used to access the page directory referenced by this entry
    u8 pcd : 1; // Page-level cache disable; indirectly determines the memory type used to access the page directory referenced by this entry
    u8 accessed : 1; //  Accessed; indicates whether this entry has been used for linear-address translation
    u8 : 1; // Ignored
    u8 page_size : 1 = 0; // Page size; must be 0 (otherwise, this entry maps a 1-GByte page)
    u8 : 3; // Ignored
    u8 r : 1; // For ordinary paging, ignored; for HLAT paging, restart (if 1, linear-address translation is restarted with ordinary paging)
    u64 paddr : VIRTUAL_ADDR_BITS - 13; // Physical address of 4-KByte aligned page directory referenced by this entry
    u64 : 51 - VIRTUAL_ADDR_BITS; // Reserved (must be 0)
    u16 : 11; // Ignored
    u8 xd : 1; // If IA32_EFER.NXE = 1, execute-disable (if 1, instruction fetches are not allowed from the 1-GByte region controlled by this entry); otherwise, reserved (must be 0)
};

// Page-Directory size: 512 entries
// Page-Directory Entry
struct PDE {
    // size: 64 bits
    u64 present : 1; // Present; must be 1 to reference a page table
    u8 rw : 1; // Read/write; if 0, writes may not be allowed to the 2-MByte region controlled by this entry
    u8 user : 1; // User/supervisor; if 0, user-mode accesses are not allowed to the 2-MByte region controlled by this entry
    u8 pwt : 1; // Page-level write-through; indirectly determines the memory type used to access the page table referenced by this entry
    u8 pcd : 1; //  Page-level cache disable; indirectly determines the memory type used to access the page table referenced by this entry
    u8 accessed : 1; // Accessed; indicates whether this entry has been used for linear-address translation
    u8 : 1; // Ignored
    u8 page_size : 1 = 0; // Page size; must be 0 (otherwise, this entry maps a 2-MByte page)
    u8 : 3; // Ignored
    u8 r : 1; // For ordinary paging, ignored; for HLAT paging, restart (if 1, linear-address translation is restarted with ordinary paging)
    u64 paddr : VIRTUAL_ADDR_BITS - 13; // Physical address of 4-KByte aligned page table referenced by this entry
    u64 : 51 - VIRTUAL_ADDR_BITS; // Reserved (must be 0)
    u16 : 11; // Ignored
    u8 xd : 1; // If IA32_EFER.NXE = 1, execute-disable (if 1, instruction fetches are not allowed from the 2-MByte region controlled by this entry); otherwise, reserved (must be 0)
};

// Page-Table size: 512 entries
// Page-Table Entry
struct PTE {
    // size: 64 bits
    u64 present : 1; // Present; must be 1 to map a 4-KByte page
    u8 rw : 1; // Read/write; if 0, writes may not be allowed to the 4-KByte page referenced by this entry
    u8 user : 1; // User/supervisor; if 0, user-mode accesses are not allowed to the 4-KByte page referenced by this entry
    u8 pwt : 1; // Page-level write-through; indirectly determines the memory type used to access the 4-KByte page referenced by this entry
    u8 pcd : 1; // Page-level cache disable; indirectly determines the memory type used to access the 4-KByte page referenced by this entry
    u8 accessed : 1; // Accessed; indicates whether software has accessed the 4-KByte page referenced by this entry
    u8 dirty : 1; // Dirty; indicates whether software has written to the 4-KByte page referenced by this entry
    u8 pat : 1; // If the PAT is supported, indirectly determines the memory type used to access the 4-KByte page referenced by this entry; otherwise, reserved (must be 0)
    u8 global : 1; // Global; if CR4.PGE = 1, determines whether the translation is global; ignored otherwise
    u8 : 2; // Ignored
    u8 r : 1; // For ordinary paging, ignored; for HLAT paging, restart (if 1, linear-address translation is restarted with ordinary paging)
    u64 paddr : VIRTUAL_ADDR_BITS - 13; // Physical address of the 4-KByte page referenced by this entry
    u8 : 7; // Ignored
    u8 pkey : 4; // Protection key; if CR4.PKE = 1 or CR4.PKS = 1, this may control the page’s access rights; otherwise, it is ignored and not used to control access rights.
    u8 xd : 1; // If IA32_EFER.NXE = 1, execute-disable (if 1, instruction fetches are not allowed from the 4-KByte page controlled by this entry); otherwise, reserved (must be 0)
};


class MMU {
public:
    friend class CPU;
    friend class Disassembler;
    MMU(CPU* cpu, size_t available_pages) : m_cpu(cpu), m_physmem_size{available_pages * PAGE_SIZE} { CPUE_ASSERT(cpu != NULL, "cpu != NULL"); }
    MMU(MMU const&) = delete;

    void init() {
        m_physmem = (u8*)mmap(NULL, m_physmem_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS, -1, 0);
        if (m_physmem == (u8*)-1) {
            fail("mmap");
        }
    }

public:
    bool mem_map();
    InterruptRaisedOr<PhysicalAddress> la_to_pa(LogicalAddress const& laddr, ByteWidth width = ByteWidth::BYTE);

    u32 mem_read32(LogicalAddress const& laddr);
    void mem_write32(LogicalAddress const& laddr, u32 value);
    u32 mem_read(LogicalAddress const& laddr, size_t width);

private:
    InterruptRaisedOr<PhysicalAddress> va_to_pa(VirtualAddress const& vaddr, ByteWidth width = ByteWidth::BYTE);
    InterruptRaisedOr<VirtualAddress> la_to_va(LogicalAddress const& laddr, ByteWidth width = ByteWidth::BYTE);
    InterruptRaisedOr<PTE*> va_to_pte(VirtualAddress const& vaddr);
    InterruptRaisedOr<void> check_page_structure(PageStructureEntry auto* entry) {
        MAY_HAVE_RAISED(check_page_structure_present(entry));
        MAY_HAVE_RAISED(check_page_structure_access_rights(entry));
        return {};
    }
    InterruptRaisedOr<void> check_page_structure_present(PageStructureEntry auto* entry);
    InterruptRaisedOr<void> check_page_structure_access_rights(PageStructureEntry auto* entry);
    template<typename T = u8>
    T* paddr_ptr(PhysicalAddress const& paddr) {
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