#include "mmio.h"

namespace CPUE {


void MMIO::map_mmio_register(PhysicalAddress const& paddr, MMIOReg const& reg) {
    TODO_NOFAIL("Maybe Alignment checks?");
    // lower bound returns end() if not found or set is empty
    auto it = m_mmio_regs.lower_bound({paddr, {}});
    if (it != m_mmio_regs.begin()) {
        --it;
        if (it->base + it->reg.width > paddr)
            fail("Trying to map overlapping MMIO registers.");
    }
    m_mmio_regs.insert(it, {paddr, reg});
}

std::pair<std::set<MMIO::MappedMMIOReg>::iterator, u8> MMIO::find_mmio_register(PhysicalAddress const& paddr) {
    auto it = m_mmio_regs.upper_bound({paddr, {}});
    if (it == m_mmio_regs.begin())
        goto not_found;
    --it;
    // if paddr is not in [base;base+width) range paddr does not correspond to any mmio reg
    if (it->base > paddr || paddr >= it->base + it->reg.width)
        goto not_found;
    CPUE_ASSERT(it->reg.read_func != nullptr, "read_func == nullptr");
    CPUE_ASSERT(it->reg.write_func != nullptr, "write_func == nullptr");
    CPUE_TRACE("Found MMIO reg for paddr 0x{:x} @ 0x{:x}", paddr.addr, it->base.addr);
    return make_pair(it, paddr.addr - it->base.addr);
not_found:
    return make_pair(m_mmio_regs.end(), 0);
}

}
