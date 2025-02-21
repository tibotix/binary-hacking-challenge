#ifndef PAGING_H
#define PAGING_H

#include "stl.h"

#define PAGE_SIZE 4096
#define PAGE_OFFSET_MASK (PAGE_SIZE - 1)
#define PAGE_NUMBER_MASK ~PAGE_OFFSET_MASK
#define PAGE_NUMBER(va) (va & PAGE_NUMBER_MASK)
#define PTE_PADDR(entry) bits(entry, 36, 12)

inline u64 minik_get_cr3(void) {
  u64 cr3;
  __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
  return cr3;
}

inline void minik_invlpg(void *addr) { __asm__("invlpg (%0)" ::"r"(addr)); }

u64 *va_to_pte(void *addr);
void remap_va_to_page_frame(void *va, u64 page_frame);
void remap_page_to_page_frame(u64 page, u64 page_frame);

#endif // PAGING_H
