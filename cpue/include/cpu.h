
#include "mmu.h"


namespace CPUE {

class CPU {
public:
    CPU() = default;
    CPU(CPU const&) = delete;

    MMU& mmu() { return m_mmu; }




private:
    MMU m_mmu;
};

}