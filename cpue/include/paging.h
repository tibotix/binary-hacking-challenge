#pragma once

#include <concepts>
#include "common.h"
#include "checked_arithmetic.h"

namespace CPUE {


/**
 * Paging:
 * With paging, portions of the linear-address space need not be mapped to the physical-address space; data for the
 * unmapped addresses can be stored externally (e.g., on disk). This method of mapping the linear-address space is
 * referred to as virtual memory or demand-paged virtual memory.
 * Paging divides the linear address space into fixed-size pages that can be mapped into the physical-address space
 * and/or external storage. When a program (or task) references a linear address, the processor uses paging to trans-
 * late the linear address into a corresponding physical address if such an address is defined.
 * If the page containing the linear address is not currently mapped into the physical-address space, the processor
 * generates a page-fault exception as described in Section 5.7. The handler for page-fault exceptions typically directs
 * the operating system or executive to load data for the unmapped page from external storage into physical memory
 * (perhaps writing a different page from physical memory out to external storage in the process) and to map it using
 * paging (by updating the paging structures). When the page has been loaded into physical memory, a return from
 * the exception handler causes the instruction that generated the exception to be restarted.
 */



// We only support one fixed PAGE_SIZE. We don't support huge pages such as 2MB Pages (referenced directly from PDE).
constexpr size_t PAGE_SIZE = 4_kb; // DO NOT CHANGE THIS
constexpr u8 VIRTUAL_ADDR_BITS = 48; // 4-Level paging enables up to 48bit VirtualAddress Space
constexpr u64 VIRTUAL_ADDR_MASK = ((u64)1 << VIRTUAL_ADDR_BITS) - 1;
constexpr u64 PAGE_NUMBER_MASK = (((u64)1 << (VIRTUAL_ADDR_BITS - 12)) - 1) << 12;
// we support Physical Memory of up to 2^36 bytes (64GiB)
constexpr u64 MAXPHYADDR = 36;
static_assert(MAXPHYADDR <= 36, "MAXPHYADDR must be <= 36");

constexpr bool IS_PAGE_ALIGNED(u64 value) {
    return ((value & ~PAGE_NUMBER_MASK) & VIRTUAL_ADDR_MASK) == 0;
}
constexpr u64 PAGE_ALIGN(u64 value) {
    return value & PAGE_NUMBER_MASK;
}
constexpr u64 PAGE_ALIGN_CEIL(u64 value) {
    return PAGE_ALIGN(CPUE_checked_uadd(value, PAGE_SIZE - 1));
}



// Base struct for any Page-Structure-Entry
union PageStructureEntry {
    struct Concrete {
        // size: 64 bits
        u64 present : 1; // Present;
        u64 rw : 1; // Read/write; if 0, writes may not be allowed to the page referenced by this entry
        u64 user : 1; // User/supervisor; if 0, user-mode accesses are not allowed to the page referenced by this entry
        u64 pwt : 1; // Page-level write-through; indirectly determines the memory type used to access the page referenced by this entry
        u64 pcd : 1; // Page-level cache disable; indirectly determines the memory type used to access the page referenced by this entry
        u64 accessed : 1; // Accessed; indicates whether software has accessed the page referenced by this entry
        u64 : 5; // Ignored
        u64 r : 1; // For ordinary paging, ignored; for HLAT paging, restart (if 1, linear-address translation is restarted with ordinary paging)
        u64 paddr : MAXPHYADDR - 12; // Physical address of the page referenced by this entry
        u64 __reserved1 : 52 - MAXPHYADDR = 0; // Reserved (must be 0)
        u64 : 11; // Ignored
        u64 xd : 1; // If IA32_EFER.NXE = 1, execute-disable (if 1, instruction fetches are not allowed from the page controlled by this entry); otherwise, reserved (must be 0)
    } c;
    u64 value;
    u64 reserved_bits_ored() const { return c.__reserved1; }
};
static_assert(sizeof(PageStructureEntry::Concrete) == 8);
static_assert(sizeof(PageStructureEntry) == 8);


// Page-Map-Level-4 size: 512 entries
// Page-Map-Level-4 Entry
union PML4E {
    struct Concrete {
        // size: 64 bits
        u64 present : 1; // Present; must be 1 to reference a page-directory-pointer table
        u64 rw : 1; // Read/write; if 0, writes may not be allowed to the 512-GByte region controlled by this entry
        u64 user : 1; // User/supervisor; if 0, user-mode accesses are not allowed to the 512-GByte region controlled by this entry
        u64 pwt : 1; // Page-level write-through; indirectly determines the memory type used to access the page-directory-pointer table referenced by this entry
        u64 pcd : 1; // Page-level cache disable; indirectly determines the memory type used to access the page-directory-pointer table referenced by this entry
        u64 accessed : 1; // Accessed; indicates whether this entry has been used for linear-address translation
        u64 : 1; // Ignored
        u64 page_size : 1 = 0; // Reserved (must be 0)
        u64 : 3; // Ignored
        u64 r : 1; // For ordinary paging, ignored; for HLAT paging, restart (if 1, linear-address translation is restarted with ordinary paging)
        u64 paddr : MAXPHYADDR - 12; // Physical address of 4-KByte aligned page-directory-pointer table referenced by this entry
        u64 __reserved1 : 52 - MAXPHYADDR = 0; // Reserved (must be 0)
        u64 : 11; // Ignored
        u64 xd : 1; // If IA32_EFER.NXE = 1, execute-disable (if 1, instruction fetches are not allowed from the 512-GByte region controlled by this entry); otherwise, reserved (must be 0)
    } c;
    u64 value;

    PML4E(u64 value = 0) : value(value) {}
    PML4E(Concrete c) : c(c) {}

    u64 reserved_bits_ored() const { return c.__reserved1; }
};
static_assert(sizeof(PML4E::Concrete) == 8);
static_assert(sizeof(PML4E) == 8);

// Page-Directory-Pointer-Table size: 512 entries
// Page-Directory-Pointer-Table Entry
union PDPTE {
    struct Concrete {
        // size: 64 bits
        u64 present : 1; // Present; must be 1 to reference a page directory
        u64 rw : 1; // Read/write; if 0, writes may not be allowed to the 1-GByte region controlled by this entry
        u64 user : 1; // User/supervisor; if 0, user-mode accesses are not allowed to the 1-GByte region controlled by this entry
        u64 pwt : 1; // Page-level write-through; indirectly determines the memory type used to access the page directory referenced by this entry
        u64 pcd : 1; // Page-level cache disable; indirectly determines the memory type used to access the page directory referenced by this entry
        u64 accessed : 1; //  Accessed; indicates whether this entry has been used for linear-address translation
        u64 : 1; // Ignored
        u64 page_size : 1 = 0; // Page size; must be 0 (otherwise, this entry maps a 1-GByte page)
        u64 : 3; // Ignored
        u64 r : 1; // For ordinary paging, ignored; for HLAT paging, restart (if 1, linear-address translation is restarted with ordinary paging)
        u64 paddr : MAXPHYADDR - 12; // Physical address of 4-KByte aligned page directory referenced by this entry
        u64 __reserved1 : 52 - MAXPHYADDR = 0; // Reserved (must be 0)
        u64 : 11; // Ignored
        u64 xd : 1; // If IA32_EFER.NXE = 1, execute-disable (if 1, instruction fetches are not allowed from the 1-GByte region controlled by this entry); otherwise, reserved (must be 0)
    } c;
    u64 value;

    PDPTE(u64 value = 0) : value(value) {}
    PDPTE(Concrete c) : c(c) {}

    u64 reserved_bits_ored() const { return c.__reserved1; }
};
static_assert(sizeof(PDPTE::Concrete) == 8);
static_assert(sizeof(PDPTE) == 8);

// Page-Directory size: 512 entries
// Page-Directory Entry
union PDE {
    struct Concrete {
        // size: 64 bits
        u64 present : 1; // Present; must be 1 to reference a page table
        u64 rw : 1; // Read/write; if 0, writes may not be allowed to the 2-MByte region controlled by this entry
        u64 user : 1; // User/supervisor; if 0, user-mode accesses are not allowed to the 2-MByte region controlled by this entry
        u64 pwt : 1; // Page-level write-through; indirectly determines the memory type used to access the page table referenced by this entry
        u64 pcd : 1; //  Page-level cache disable; indirectly determines the memory type used to access the page table referenced by this entry
        u64 accessed : 1; // Accessed; indicates whether this entry has been used for linear-address translation
        u64 : 1; // Ignored
        u64 page_size : 1 = 0; // Page size; must be 0 (otherwise, this entry maps a 2-MByte page)
        u64 : 3; // Ignored
        u64 r : 1; // For ordinary paging, ignored; for HLAT paging, restart (if 1, linear-address translation is restarted with ordinary paging)
        u64 paddr : MAXPHYADDR - 12; // Physical address of 4-KByte aligned page table referenced by this entry
        u64 __reserved1 : 52 - MAXPHYADDR = 0; // Reserved (must be 0)
        u64 : 11; // Ignored
        u64 xd : 1; // If IA32_EFER.NXE = 1, execute-disable (if 1, instruction fetches are not allowed from the 2-MByte region controlled by this entry); otherwise, reserved (must be 0)
    } c;
    u64 value;

    PDE(u64 value = 0) : value(value) {}
    PDE(Concrete c) : c(c) {}

    u64 reserved_bits_ored() const { return c.__reserved1; }
};
static_assert(sizeof(PDE::Concrete) == 8);
static_assert(sizeof(PDE) == 8);

// Page-Table size: 512 entries
// Page-Table Entry
union PTE {
    struct Concrete {
        // size: 64 bits
        u64 present : 1; // Present; must be 1 to map a 4-KByte page
        u64 rw : 1; // Read/write; if 0, writes may not be allowed to the 4-KByte page referenced by this entry
        u64 user : 1; // User/supervisor; if 0, user-mode accesses are not allowed to the 4-KByte page referenced by this entry
        u64 pwt : 1; // Page-level write-through; indirectly determines the memory type used to access the 4-KByte page referenced by this entry
        u64 pcd : 1; // Page-level cache disable; indirectly determines the memory type used to access the 4-KByte page referenced by this entry
        u64 accessed : 1; // Accessed; indicates whether software has accessed the 4-KByte page referenced by this entry
        u64 dirty : 1; // Dirty; indicates whether software has written to the 4-KByte page referenced by this entry
        u64 pat : 1; // If the PAT is supported, indirectly determines the memory type used to access the 4-KByte page referenced by this entry; otherwise, reserved (must be 0)
        u64 global : 1; // Global; if CR4.PGE = 1, determines whether the translation is global; ignored otherwise
        u64 : 2; // Ignored
        u64 r : 1; // For ordinary paging, ignored; for HLAT paging, restart (if 1, linear-address translation is restarted with ordinary paging)
        u64 paddr : MAXPHYADDR - 12; // Physical address of the 4-KByte page referenced by this entry
        u64 __reserved1 : 52 - MAXPHYADDR = 0; // Reserved (must be 0)
        u64 : 7; // Ignored
        u64 pkey : 4; // Protection key; if CR4.PKE = 1 or CR4.PKS = 1, this may control the page’s access rights; otherwise, it is ignored and not used to control access rights.
        u64 xd : 1; // If IA32_EFER.NXE = 1, execute-disable (if 1, instruction fetches are not allowed from the 4-KByte page controlled by this entry); otherwise, reserved (must be 0)
    } c;
    u64 value;

    PTE(u64 value = 0) : value(value) {}
    PTE(Concrete c) : c(c) {}

    u64 reserved_bits_ored() const { return c.__reserved1; }
};
static_assert(sizeof(PTE::Concrete) == 8);
static_assert(sizeof(PTE) == 8);


}