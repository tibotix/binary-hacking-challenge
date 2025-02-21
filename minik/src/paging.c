#include <minik/paging.h>

u64 *va_to_pte(void *addr) {
  u64 addr_val = (u64)addr;
  u64 cr3 = minik_get_cr3();
  u64 pml4e = *(u64 *)(PTE_PADDR(cr3) << 12 | bits(addr_val, 47, 39) << 3);
  u64 pdpte = *(u64 *)(PTE_PADDR(pml4e) << 12 | bits(addr_val, 38, 30) << 3);
  u64 pde = *(u64 *)(PTE_PADDR(pdpte) << 12 | bits(addr_val, 29, 21) << 3);
  u64 *pte = (u64 *)(PTE_PADDR(pde) << 12 | bits(addr_val, 20, 12) << 3);
  return pte;
}

void remap_va_to_page_frame(void *va, u64 page_frame) {
  u64 *pte_ptr = va_to_pte(va);
  u64 pte = *pte_ptr;
  *pte_ptr = bits(pte, 63, 37) << 36 | (page_frame) << 12 | bits(pte, 11, 0);
  minik_invlpg(va);
}
void remap_page_to_page_frame(u64 page, u64 page_frame) {
  void *va = (void *)page;
  return remap_va_to_page_frame(va, page_frame);
}
