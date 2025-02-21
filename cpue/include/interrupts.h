#pragma once

#include "common.h"
#include <optional>


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
requires std::is_void_v<T> || std::is_default_constructible_v<T>
class InterruptRaisedOr {
public:
    InterruptRaisedOr(_InterruptRaised) : InterruptRaisedOr<T>(true, {}){};
    template<typename U = T>
    InterruptRaisedOr(U&& value) : InterruptRaisedOr<T>(false, std::forward<U>(value)){};
    static InterruptRaisedOr<T> InterruptRaised() { return {true, {}}; }

    bool raised() const { return m_raised; }
    T release_value() {
        CPUE_ASSERT(!raised(), "Trying to call release_value() with raised()==true.");
        return std::move(m_value);
    }

private:
    template<typename U = T>
    InterruptRaisedOr(bool raised, U&& value) : m_raised(raised), m_value(std::forward<U>(value)) {}
    bool m_raised;
    T m_value;
};

template<>
class InterruptRaisedOr<void> {
public:
    InterruptRaisedOr(_InterruptRaised) : InterruptRaisedOr<void>(true){};
    InterruptRaisedOr() : InterruptRaisedOr<void>(false){};
    static InterruptRaisedOr<void> InterruptRaised() { return {true}; }

    bool raised() const { return m_raised; }
    void release_value() { CPUE_ASSERT(!raised(), "Trying to call release_value() with raised()==true."); }

private:
    InterruptRaisedOr(bool raised) : m_raised(raised) {}
    bool m_raised;
};




struct ErrorCode {
    // size: 32 bits
    union {
        // standard error-code format
        struct {
            /**
             * When set, indicates that the exception occurred during delivery of an
             * event external to the program, such as an interrupt or an earlier exception (also on INT1).
             * The bit is cleared if the exception occurred during delivery of a software interrupt (INT n, INT3, or INTO).
             */
            u32 ext : 1 = 0;
            /**
             * 0b00	The Selector Index references a descriptor in the GDT.
             * 0b01	The Selector Index references a descriptor in the IDT.
             * 0b10	The Selector Index references a descriptor in the LDT.
             * 0b11	The Selector Index references a descriptor in the IDT.
             */
            u32 tbl : 2;
            u32 selector_index : 13; // Segment Selector Index
        } standard;

        // page-fault specific error-code format
        struct {
            /**
             *  0 The fault was caused by a non-present page.
             *  1 The fault was caused by a page-level protection violation.
            */
            u32 present : 1;
            /**
             * 0 The access causing the fault was a read.
             * 1 The access causing the fault was a write.
             */
            u32 wr : 1;
            /**
             * 0 A supervisor-mode access caused the fault.
             * 1 A user-mode access caused the fault.
             */
            u32 us : 1;
            /**
             * 0 The fault was not caused by reserved bit violation.
             * 1 The fault was caused by a reserved bit set to 1 in some
             * paging-structure entry.
             */
            u32 reserved : 1;
            /**
             * 0 The fault was not caused by an instruction fetch.
             * 1 The fault was caused by an instruction fetch.
             */
            u32 id : 1;
            /**
             * 0 The fault was not caused by protection keys.
             * 1 There was a protection-key violation.
             */
            u32 pk : 1 = 0;
            /**
             * 0 The fault was not caused by a shadow-stack access.
             * 1 The fault was caused by a shadow-stack access.
             */
            u32 ss : 1 = 0;
            /**
             * 0 The fault occurred during ordinary paging or due to access rights.
             * 1 The fault occurred during HLAT paging.
             */
            u32 hlat : 1 = 0;
            u32 : 7; // Reserved
            /**
             * 0 The fault is not related to SGX.
             * 1 The fault resulted from violation of SGX-specific access-control requirements.
             */
            u32 sgx : 1 = 0;
        } pf;
        // control-protection-exception specific error-code format
        struct {
            /**
             * value - meaning:
             * 1 - NEAR-RET: Indicates the #CP was caused by a near RET instruction.
             * 2 - FAR-RET/IRET: Indicates the #CP was caused by a FAR RET or IRET instruction.
             * 3 - ENDBRANCH: indicates the #CP was due to missing ENDBRANCH at target of an indirect call or jump instruction.
             * 4 - RSTORSSP: Indicates the #CP was caused by a shadow-stack-restore token check failure in the RSTORSSP instruction.
             * 5-  SETSSBSY: Indicates #CP was caused by a supervisor shadow stack token check failure in the SETSSBSY instruction.
             */
            u32 cpec : 15;
            // if set to 1, indicates the #CP occurred during enclave execution.
            u32 encl : 1;
        } cp;
        u32 value;
    };
};
static_assert(sizeof(ErrorCode) == 4);
/**
 * Zero error code for exceptions that occurred during delivery of a software interrupt (INT n, INT3, INTO);
 */
constexpr ErrorCode ZERO_ERROR_CODE_NOEXT = {
    .standard =
        {
            .ext = 0,
            .tbl = 0,
            .selector_index = 0,
        },
};
/**
 * Zero error code for exceptions that occurred during delivery of an event external to the program,
 * such as an interrupt or an earlier exception (including INT1).
 */
constexpr ErrorCode ZERO_ERROR_CODE_EXT = {
    .standard =
        {
            .ext = 1,
            .tbl = 0,
            .selector_index = 0,
        },
};




enum InterruptClass : u8 {
    BENIGN = 0, // NOTE: all interrupts are benign (also external interrupts coming from INTR/NMI pin)
    CONTRIBUTORY,
    PAGE_FAULT,
    DOUBLE_FAULT,
};

enum InterruptCategory {
    /**
     * processor-generated, error code is pushed if the exception has one
     */
    EXCEPTION,
    /**
     * externally-generated or software-generated, error code is not pushed even if
     * INT n instruction references an IDT entry normally used by exceptions that have an error code.
     */
    INTERRUPT,
};

class InterruptType {
public:
    // Exceptions push an error code depending on their spec
    // Interrupts do not push an error code
    enum _InterruptType {
        /**
         * Delivered through an internal mechanism in the CPU.
         * A fault is an exception that can generally be corrected and that, once corrected, allows the program
         * to be restarted with no loss of continuity. When a fault is reported, the processor restores the machine state to
         * the state prior to the beginning of execution of the faulting instruction. The return address (saved contents of
         * the CS and EIP registers) for the fault handler points to the faulting instruction, rather than to the instruction
         * following the faulting instruction.
         */
        FAULT_EXCEPTION,
        /**
         * Delivered through an internal mechanism in the CPU.
         * A trap is an exception that is reported immediately following the execution of the trapping instruction.
         * Traps allow execution of a program or task to be continued without loss of program continuity. The return
         * address for the trap handler points to the instruction to be executed after the trapping instruction.
         */
        TRAP_EXCEPTION,
        /**
         * Delivered through an internal mechanism in the CPU.
         * An abort is an exception that does not always report the precise location of the instruction causing
         * the exception and does not allow a restart of the program or task that caused the exception. Aborts are used to
         * report severe errors, such as hardware errors and inconsistent or illegal values in system tables.
         */
        ABORT_EXCEPTION,
        /**
         * Delivered through the NMI Pin or through INTR pin / local APIC with a delivery mode NMI
         * When the processor receives a NMI from either of these sources, the processor handles it immediately by calling
         * the NMI handler pointed to by interrupt vector number 2. The processor also invokes certain hardware conditions
         * to ensure that no other interrupts, including NMI interrupts, are received until the NMI handler has completed
         * executing
         */
        NON_MASKABLE_INTERRUPT,
        /**
         * Delivered through the INTR pin / local APIC with a delivery mode of not NMI
         * These Interrupts can be masked by the IF flag.
         */
        MASKABLE_INTERRUPT,
        /**
         * Interrupts generated by software.
         * These include the INT n, as well as the INTO, INT1, INT3, and BOUND instructions.
         * These Interrupts cannot be masked by the IF flag int the EFLAGS register.
         */
        SOFTWARE_INTERRUPT,
    };

    InterruptType() = default;
    constexpr InterruptType(_InterruptType type) : m_type(type) {}

    // Allow switch and comparisons.
    constexpr operator _InterruptType() const { return m_type; }

    // Prevent usage in if statement
    explicit operator bool() const = delete;

    [[nodiscard]] constexpr InterruptCategory category() const {
        switch (m_type) {
            case _InterruptType::FAULT_EXCEPTION:
            case _InterruptType::TRAP_EXCEPTION:
            case _InterruptType::ABORT_EXCEPTION: return InterruptCategory::EXCEPTION;
            case _InterruptType::NON_MASKABLE_INTERRUPT:
            case _InterruptType::MASKABLE_INTERRUPT:
            case _InterruptType::SOFTWARE_INTERRUPT: return InterruptCategory::INTERRUPT;
            default: fail();
        }
    }

private:
    _InterruptType m_type;
};


class InterruptSource {
public:
    enum _InterruptSource {
        INTR_PIN,
        NMI_PIN,
        INT1_INSN,
        INTN_INT3_INTO_INSN,
        INTERNAL,
    };
    InterruptSource() = default;
    constexpr InterruptSource(_InterruptSource source) : m_source(source) {}

    // See ErrorCode.ext field
    [[nodiscard]] bool is_external() const {
        switch (m_source) {
            case INTR_PIN:
            case NMI_PIN:
            case INT1_INSN: return true;
            case INTN_INT3_INTO_INSN:
            case INTERNAL: return false;
            default: fail();
        }
    }

    // Allow switch and comparisons.
    constexpr operator _InterruptSource() const { return m_source; }

    // Prevent usage in if statement
    explicit operator bool() const = delete;

private:
    _InterruptSource m_source;
};


struct Interrupt {
    InterruptVector vector;
    InterruptType type;
    InterruptClass iclass;
    InterruptSource source;
    // Error codes are only used for exceptions
    std::optional<ErrorCode> error_code;
};




class Exceptions {
public:
    static constexpr Interrupt DE() { return {VEC_DE, InterruptType::FAULT_EXCEPTION, InterruptClass::CONTRIBUTORY, InterruptSource::INTERNAL}; }
    static constexpr Interrupt DB() { return {VEC_DB, InterruptType::FAULT_EXCEPTION, InterruptClass::BENIGN, InterruptSource::INTERNAL}; }
    static constexpr Interrupt NMI() { return {VEC_NMI, InterruptType::NON_MASKABLE_INTERRUPT, InterruptClass::BENIGN, InterruptSource::INTERNAL}; }
    static constexpr Interrupt BP() { return {VEC_BP, InterruptType::TRAP_EXCEPTION, InterruptClass::BENIGN, InterruptSource::INTERNAL}; }
    static constexpr Interrupt OF() { return {VEC_OF, InterruptType::TRAP_EXCEPTION, InterruptClass::BENIGN, InterruptSource::INTERNAL}; }
    static constexpr Interrupt BR() { return {VEC_BR, InterruptType::FAULT_EXCEPTION, InterruptClass::BENIGN, InterruptSource::INTERNAL}; }
    static constexpr Interrupt UD() { return {VEC_UD, InterruptType::FAULT_EXCEPTION, InterruptClass::BENIGN, InterruptSource::INTERNAL}; }
    static constexpr Interrupt NM() { return {VEC_NM, InterruptType::FAULT_EXCEPTION, InterruptClass::BENIGN, InterruptSource::INTERNAL}; }
    static constexpr Interrupt DF(ErrorCode error_code = ZERO_ERROR_CODE_NOEXT) {
        return {VEC_DF, InterruptType::ABORT_EXCEPTION, InterruptClass::DOUBLE_FAULT, InterruptSource::INTERNAL, error_code};
    }
    static constexpr Interrupt TS(ErrorCode error_code) { return {VEC_TS, InterruptType::FAULT_EXCEPTION, InterruptClass::CONTRIBUTORY, InterruptSource::INTERNAL, error_code}; }
    static constexpr Interrupt NP(ErrorCode error_code) { return {VEC_NP, InterruptType::FAULT_EXCEPTION, InterruptClass::CONTRIBUTORY, InterruptSource::INTERNAL, error_code}; }
    static constexpr Interrupt SS(ErrorCode error_code) { return {VEC_SS, InterruptType::FAULT_EXCEPTION, InterruptClass::CONTRIBUTORY, InterruptSource::INTERNAL, error_code}; }
    static constexpr Interrupt GP(ErrorCode error_code) { return {VEC_GP, InterruptType::FAULT_EXCEPTION, InterruptClass::CONTRIBUTORY, InterruptSource::INTERNAL, error_code}; }
    static constexpr Interrupt PF(ErrorCode error_code = ZERO_ERROR_CODE_NOEXT) {
        return {VEC_PF, InterruptType::FAULT_EXCEPTION, InterruptClass::PAGE_FAULT, InterruptSource::INTERNAL, error_code};
    }
    static constexpr Interrupt MF() { return {VEC_MF, InterruptType::FAULT_EXCEPTION, InterruptClass::BENIGN, InterruptSource::INTERNAL}; }
    static constexpr Interrupt AC(ErrorCode error_code = ZERO_ERROR_CODE_NOEXT) {
        return {VEC_AC, InterruptType::FAULT_EXCEPTION, InterruptClass::BENIGN, InterruptSource::INTERNAL, error_code};
    }
    static constexpr Interrupt MC() { return {VEC_MC, InterruptType::ABORT_EXCEPTION, InterruptClass::BENIGN, InterruptSource::INTERNAL}; }
    static constexpr Interrupt XM() { return {VEC_XM, InterruptType::FAULT_EXCEPTION, InterruptClass::BENIGN, InterruptSource::INTERNAL}; }
    static constexpr Interrupt VE() { return {VEC_VE, InterruptType::FAULT_EXCEPTION, InterruptClass::PAGE_FAULT, InterruptSource::INTERNAL}; }
    static constexpr Interrupt CP(ErrorCode error_code = ZERO_ERROR_CODE_NOEXT) {
        return {VEC_CP, InterruptType::FAULT_EXCEPTION, InterruptClass::CONTRIBUTORY, InterruptSource::INTERNAL, error_code};
    }

    static constexpr InterruptVector VEC_DE = 0; // Divide Error
    static constexpr InterruptVector VEC_DB = 1; // Debug
    static constexpr InterruptVector VEC_NMI = 2; // NonMaskable external interrupt
    static constexpr InterruptVector VEC_BP = 3; // Breakpoint
    static constexpr InterruptVector VEC_OF = 4; // Overflow
    static constexpr InterruptVector VEC_BR = 5; // BOUND Range Exceeded
    static constexpr InterruptVector VEC_UD = 6; // Invalid Opcode (Undefined Opcode)
    static constexpr InterruptVector VEC_NM = 7; // Device Not Available
    static constexpr InterruptVector VEC_DF = 8; // Double Fault
    static constexpr InterruptVector VEC_TS = 10; // Invalid TSS
    static constexpr InterruptVector VEC_NP = 11; // Segment Not Present
    static constexpr InterruptVector VEC_SS = 12; // Stack Segment Fault
    static constexpr InterruptVector VEC_GP = 13; // General Protection
    static constexpr InterruptVector VEC_PF = 14; // Page Fault
    static constexpr InterruptVector VEC_MF = 16; // Floating-Point Error (Math Fault)
    static constexpr InterruptVector VEC_AC = 17; // Alignment Check
    static constexpr InterruptVector VEC_MC = 18; // Machine Check
    static constexpr InterruptVector VEC_XM = 19; // SIMD Floating-Point Exception
    static constexpr InterruptVector VEC_VE = 20; // Virtualization Exception
    static constexpr InterruptVector VEC_CP = 21; // Control Protection Exception
};


}