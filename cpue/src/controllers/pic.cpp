
#include "controllers/pic.h"
#include "controllers/icu.h"

namespace CPUE {


void PIC::mask_pin(u8 pin) {
    CPUE_ASSERT(pin < PIC_NUM_IRQ_PINS, "Invalid pin number");
    m_imr.set(pin);
}
void PIC::unmask_pin(u8 pin) {
    CPUE_ASSERT(pin < PIC_NUM_IRQ_PINS, "Invalid pin number");
    m_imr.reset(pin);
}

void PIC::raise_irq_pin(PICConnectionHandle const& handle) {
    if (m_imr[handle.pin()])
        return;
    m_irr.set(handle.pin());
    process_pending_irqs();
}
void PIC::clear_irq_pin(PICConnectionHandle const& handle) {
    m_irr.reset(handle.pin());
    process_pending_irqs();
}

void PIC::process_pending_irqs() {
    // if any interrupt is currently in-service, don't request another
    if (m_isr.any())
        return;
    for (u8 i = 0; i < m_irr.size(); ++i) {
        // if pin is not requesting interrupt, continue
        if (m_irr[i] == 0)
            continue;
        // if interrupt is masked, don't process it
        if (m_imr[i] == 1)
            continue;
        // NOTE: we do not implement priorities
        // we have an interrupt candidate -> send it to cpu and wait for acknowledgement
        Interrupt interrupt = {
            .vector = static_cast<u8>(PIC_IRQ_VEC_BASE + i),
            .type = InterruptType::MASKABLE_INTERRUPT,
            .iclass = InterruptClass::BENIGN,
            .source = InterruptSource::INTR_PIN,
        };
        auto intr = m_icu->intr_raise_interrupt(interrupt);
        // if we didn't raise, icu doesn't acknowledged it.
        if (!intr.raised())
            continue;
        m_irr.reset(i);
        m_isr.set(i);
    }
    TODO_NOFAIL("wait for EOI and then clear m_isr.");
}

}