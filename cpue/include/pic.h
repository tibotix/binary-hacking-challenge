#pragma once

#include <optional>
#include <queue>
#include <array>
#include "common.h"
#include "interrupts.h"

namespace CPUE {


// Programmable Interrupt Controller
class PIC {
public:
    PIC() = default;
    PIC(PIC const&) = delete;

public:
    static constexpr u8 MAX_PENDING_CAPACITY = 32;
    // TODO: add error_code functionality
    _InterruptRaised deliver_interrupt(Interrupt i, u8 priority) {
        CPUE_ASSERT(m_pending_interrupts.size() < MAX_PENDING_CAPACITY, "PIC capacity full");
        m_pending_interrupts.push({i, priority});
        return INTERRUPT_RAISED;
    }
    _InterruptRaised deliver_interrupt(Interrupt i) {
        TODO_NOFAIL("Implement proper interrupt priority setting");
        u8 priority = [&]() -> u8 {
            if (i == Exceptions::MC)
                return 1;
            switch (i.type) {
                case InterruptType::FAULT_EXCEPTION: return 7; break;
                case InterruptType::TRAP_EXCEPTION: return 4; break;
                case InterruptType::ABORT_EXCEPTION: return 0; break;
                case InterruptType::NON_MASKABLE_INTERRUPT: return 5; break;
                case InterruptType::SOFTWARE_INTERRUPT: return 9; break;
                default: fail("Unhandled InterruptType");
            }
        }();
        return deliver_interrupt(i, priority);
    }
    _InterruptRaised deliver_software_interrupt(InterruptVector ivec) { return deliver_interrupt({ivec, InterruptType::SOFTWARE_INTERRUPT, false}); }
    void clear_interrupts() {
        // std::priority_queue doesn't have a clear method -> just re-assign it
        m_pending_interrupts = decltype(m_pending_interrupts){_comparator};
    }
    std::optional<Interrupt> pending_interrupt_with_highest_priority() {
        if (m_pending_interrupts.empty())
            return {};
        auto prioritized_interrupt = m_pending_interrupts.top();
        m_pending_interrupts.pop();
        return prioritized_interrupt.interrupt;
    }

private:
    struct _PrioritizedInterrupt {
        Interrupt interrupt;
        u8 priority;
    };
    struct {
        bool operator()(_PrioritizedInterrupt const l, _PrioritizedInterrupt const r) const { return l.priority < r.priority; }
    } _comparator;
    std::priority_queue<_PrioritizedInterrupt, std::vector<_PrioritizedInterrupt>, decltype(_comparator)> m_pending_interrupts{_comparator};
};

}