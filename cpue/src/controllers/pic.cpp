
#include "controllers/pic.h"
#include "cpu.h"

namespace CPUE {



PICConnectionHandle::PICConnectionHandle(u8 pin) : m_pin(pin) {
    CPUE_ASSERT(m_pin < PIC::NUM_IRQ_PINS, "Pin number too big");
}

void PIC::init_mmio_registers() {
    // EOI write trigger
    m_cpu->mmu().mmio().map_mmio_register(MMIO_REG_BASE + 0x0, {
                                                                   .width = ByteWidth::WIDTH_BYTE,
                                                                   .read_func = [](void* data) -> BigEndian<u64> {
                                                                       fail("Trying to read non-readable MMIO register");
                                                                   },
                                                                   .write_func =
                                                                       [](void* data, const BigEndian<u64> value) {
                                                                           auto* this_ = static_cast<PIC*>(data);
                                                                           this_->received_eoi();
                                                                       },
                                                                   .data = this,
                                                               });

    // ICW4
    m_cpu->mmu().mmio().map_mmio_register(MMIO_REG_BASE + 0x1, {
                                                                   .width = ByteWidth::WIDTH_BYTE,
                                                                   .read_func = [](void* data) -> BigEndian<u64> {
                                                                       auto* this_ = static_cast<PIC*>(data);
                                                                       std::scoped_lock _(this_->m_mutex);
                                                                       return this_->m_icw4.value;
                                                                   },
                                                                   .write_func =
                                                                       [](void* data, const BigEndian<u64> value) {
                                                                           auto* this_ = static_cast<PIC*>(data);
                                                                           std::scoped_lock _(this_->m_mutex);
                                                                           this_->m_icw4.value = value.value;
                                                                       },
                                                                   .data = this,
                                                               });
}



void PIC::mask_pin(u8 pin) {
    CPUE_ASSERT(pin < NUM_IRQ_PINS, "Invalid pin number");
    std::scoped_lock _(m_mutex);
    m_imr.set(pin);
}
void PIC::unmask_pin(u8 pin) {
    CPUE_ASSERT(pin < NUM_IRQ_PINS, "Invalid pin number");
    std::scoped_lock _(m_mutex);
    m_imr.reset(pin);
}

void PIC::raise_irq_pin(PICConnectionHandle const& handle) {
    std::scoped_lock _(m_mutex);
    if (m_imr[handle.pin()])
        return;
    m_irr.set(handle.pin());
    process_pending_irqs();
}
void PIC::clear_irq_pin(PICConnectionHandle const& handle) {
    std::scoped_lock _(m_mutex);
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
            .vector = static_cast<u8>(IRQ_VEC_BASE + i),
            .type = InterruptType::MASKABLE_INTERRUPT,
            .iclass = InterruptClass::BENIGN,
            .source = InterruptSource::INTR_PIN,
        };
        auto intr = m_cpu->icu().intr_raise_interrupt(interrupt);
        // if we didn't raise, icu didn't acknowledge it.
        if (!intr.raised())
            continue;
        m_irr.reset(i);
        // if not using AEOI, set the isr bit and wait for EOI from OS to clear it.
        if (!m_icw4.concrete.aeoi)
            m_isr.set(i);
    }
}

void PIC::received_eoi() {
    std::scoped_lock _(m_mutex);
    m_isr.reset();
}

}