#pragma once

#include "common.h"


#define MAY_HAVE_RAISED(expression)                  \
    ({                                               \
        auto&& _temporary_result = (expression);     \
        if (_temporary_result.raised()) [[unlikely]] \
            return INTERRUPT_RAISED;                 \
        _temporary_result.release_value();           \
    })



namespace CPUE {

// See Section 7.0 or Intel Manual page 3280

typedef u8 InterruptVector;

class _InterruptRaised {
public:
    explicit constexpr _InterruptRaised() {}
};
constexpr _InterruptRaised INTERRUPT_RAISED;

template<typename T>
class InterruptRaisedOr {
public:
    InterruptRaisedOr(_InterruptRaised) : InterruptRaisedOr<T>(true, {}) {};
    template<typename U = T>
    InterruptRaisedOr(U&& value) : InterruptRaisedOr<T>(false, std::move(value)){};
    static InterruptRaisedOr<T> InterruptRaised() { return {true, {}}; }

    bool raised() const { return m_raised; }
    T release_value() { return std::move(m_value); }

private:
    InterruptRaisedOr(bool raised, T&& value) : m_raised(raised), m_value(std::move(value)) {}
    bool m_raised;
    T m_value;
};

template<>
class InterruptRaisedOr<void> {
public:
    InterruptRaisedOr(_InterruptRaised) : InterruptRaisedOr<void>(true) {};
    InterruptRaisedOr() : InterruptRaisedOr<void>(false) {};
    static InterruptRaisedOr<void> InterruptRaised() { return {true}; }

    bool raised() const { return m_raised; }
    void release_value() {}

private:
    InterruptRaisedOr(bool raised) : m_raised(raised) {}
    bool m_raised;
};



enum InterruptType {
    FAULT_EXCEPTION, // Does allow restart of insn
    TRAP_EXCEPTION, // Does allow restart of insn
    ABORT_EXCEPTION, // Does not allow restart
    NON_MASKABLE_INTERRUPT,
    SOFTWARE_INTERRUPT,
};


// Interrupts generated in software with the INT n instruction cannot be masked by the IF flag in the EFLAGS register


struct Interrupt {
    InterruptVector vector;
    InterruptType type;
    bool has_error_code;

    bool operator==(Interrupt const& other) const = default;
};

class Exceptions {
public:
    static constexpr Interrupt DE = {0, InterruptType::FAULT_EXCEPTION, false}; // Divide Error
    static constexpr Interrupt DB = {1, InterruptType::FAULT_EXCEPTION, false}; // Debug
    static constexpr Interrupt NMI = {2, InterruptType::NON_MASKABLE_INTERRUPT, false}; // NonMaskable external interrupt
    static constexpr Interrupt BP = {3, InterruptType::TRAP_EXCEPTION, false}; // Breakpoint
    static constexpr Interrupt OF = {4, InterruptType::TRAP_EXCEPTION, false}; // Overflow
    static constexpr Interrupt BR = {5, InterruptType::FAULT_EXCEPTION, false}; // BOUND Range Exceeded
    static constexpr Interrupt UD = {6, InterruptType::FAULT_EXCEPTION, false}; // Invalid Opcode (Undefined Opcode)
    static constexpr Interrupt NM = {7, InterruptType::FAULT_EXCEPTION, false}; // Device Not Available
    static constexpr Interrupt DF = {8, InterruptType::ABORT_EXCEPTION, true}; // Double Fault
    static constexpr Interrupt TS = {10, InterruptType::FAULT_EXCEPTION, true}; // Invalid TSS
    static constexpr Interrupt NP = {11, InterruptType::FAULT_EXCEPTION, true}; // Segment Not Present
    static constexpr Interrupt SS = {12, InterruptType::FAULT_EXCEPTION, true}; // Stack Segment Fault
    static constexpr Interrupt GP = {13, InterruptType::FAULT_EXCEPTION, true}; // General Protection
    static constexpr Interrupt PF = {14, InterruptType::FAULT_EXCEPTION, true}; // Page Fault
    static constexpr Interrupt MF = {16, InterruptType::FAULT_EXCEPTION, false}; // Floating-Point Error (Math Fault)
    static constexpr Interrupt AC = {17, InterruptType::FAULT_EXCEPTION, true}; // Alignment Check
    static constexpr Interrupt MC = {18, InterruptType::ABORT_EXCEPTION, false}; // Machine Check
    static constexpr Interrupt XM = {19, InterruptType::FAULT_EXCEPTION, false}; // SIMD Floating-Point Exception
    static constexpr Interrupt VE = {20, InterruptType::FAULT_EXCEPTION, false}; // Virtualization Exception
    static constexpr Interrupt CP = {21, InterruptType::FAULT_EXCEPTION, true}; // Control Protection Exception
};


}