#pragma once

#include <mutex>
#include <optional>
#include <queue>

#include "common.h"
#include "interrupts.h"
#include "forward.h"


namespace CPUE {

/**
 * Interrupt Controller Unit
 * This class encapsulates functionality related to interrupts and exceptions.
 * It acts as a centralized location to receive interrupts from:
 *  - the processor itself (processor-generated exceptions)
 *  - NMI pin
 *  - INTR pin
 */
class ICU {
public:
    friend CPU;
    ICU(CPU* cpu) : m_cpu(cpu) {}
    ICU(ICU const&) = delete;

    bool intr_pin_enabled() const;
    _InterruptRaised nmi_raise_interrupt(Interrupt i) { fail("NMI pin is not implemented"); }
    InterruptRaisedOr<void> intr_raise_interrupt(Interrupt i) {
        CPUE_ASSERT(i.source == InterruptSource::INTR_PIN, "Non INTR_PIN interrupt raised through intr_raise_interrupt.");
        if (!intr_pin_enabled())
            return {};
        return raise_interrupt(i);
    }

private:
    static constexpr u8 MAX_PENDING_CAPACITY = 127;
    _InterruptRaised raise_interrupt(Interrupt i, u8 priority) {
        CPUE_ASSERT(1 <= priority && priority <= 9, "priority must be between 1 and 9");
        return _raise_interrupt(i, priority);
    }
    _InterruptRaised raise_interrupt(Interrupt i) {
        TODO_NOFAIL("Implement proper interrupt priority setting");
        u8 priority = [&]() -> u8 {
            switch (i.type) {
                case InterruptType::ABORT_EXCEPTION: return 1;
                case InterruptType::TRAP_EXCEPTION: return 2;
                case InterruptType::FAULT_EXCEPTION: return 7;
                case InterruptType::NON_MASKABLE_INTERRUPT: return 5;
                case InterruptType::MASKABLE_INTERRUPT: return 6;
                default: return 9;
            }
        }();
        return raise_interrupt(i, priority);
    }

    /**
     * Instruction integral interrupts are handled as an integral part of the current instruction,
     * so they get a virtual highest priority.
     * NOTE: If #PF and #GP occur during fetching of an instruction, they are not integral.
     *       If #GP occur during decoding of an instruction (cause Instruction length > 15 bytes), they are not integral.
     *       Otherwise, they are integral.
     * Integral Exceptions are: DE, BP, OF, BR, TS, NP, SS, GP, PF, AC, MF, XM, VE, CP,
     */
    _InterruptRaised raise_integral_interrupt(Interrupt i) { return _raise_interrupt(i, 0); }

    std::optional<Interrupt> pop_highest_priority_interrupt() {
        std::scoped_lock lock(m_lock);
        if (m_pending_interrupts.empty())
            return {};
        auto prioritized_interrupt = m_pending_interrupts.top();
        m_pending_interrupts.pop();
        return prioritized_interrupt.interrupt;
    }

    _InterruptRaised _raise_interrupt(Interrupt i, u8 priority) {
        std::scoped_lock lock(m_lock);
        CPUE_ASSERT(m_pending_interrupts.size() < MAX_PENDING_CAPACITY, "ICU capacity full");
        m_pending_interrupts.push({i, priority});
        return INTERRUPT_RAISED;
    }

    struct _PrioritizedInterrupt {
        Interrupt interrupt;
        u8 priority;
    };
    struct {
        bool operator()(_PrioritizedInterrupt const l, _PrioritizedInterrupt const r) const { return l.priority < r.priority; }
    } _comparator;
    std::priority_queue<_PrioritizedInterrupt, std::vector<_PrioritizedInterrupt>, decltype(_comparator)> m_pending_interrupts{_comparator};
    std::mutex m_lock;
    CPU* m_cpu;
};

}