#include <cstdlib>
#include <stdint.h>
#include <sys/mman.h>

#include "common.h"
#include "tlb.h"
#include "address.h"


namespace CPUE {


class MMU {
public:
    MMU(size_t available_ram) : m_physmem_size{available_ram} {}
    MMU(MMU const&) = delete;

    void init() {
        m_physmem = (u8*)mmap(NULL, m_physmem_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS, -1, 0);
        if (m_physmem == (u8*)-1) {
            fail("mmap");
        }
    }

    bool mem_map();
    u32 mem_read32(LogicalAddress const& addr);
    void mem_write32(LogicalAddress const& addr, u32 value);



public:
private:
    TLB m_tlb;
    size_t m_physmem_size;
    u8* m_physmem;
};

}