#include "controllers/icu.h"
#include "cpu.h"

namespace CPUE {


bool ICU::intr_pin_enabled() const {
    return m_cpu->m_rflags.IF;
}

}