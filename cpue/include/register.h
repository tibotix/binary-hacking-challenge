#pragma once

#include <capstone/x86.h>
#include "common.h"
#include "sized_value.h"
#include "interrupts.h"

namespace CPUE {


class RegisterModifier {
public:
    static RegisterModifier QWORD;
    static RegisterModifier DWORD;
    static RegisterModifier WORD;
    static RegisterModifier LOW;
    static RegisterModifier HIGH;

    [[nodiscard]] SizedValue read(u64 const* value_ptr) const { return {(*value_ptr & m_bitmask) >> m_rshift, m_width}; }
    void write(u64* value_ptr, SizedValue const& svalue) {
        CPUE_ASSERT(svalue.width() <= m_width, "Invalid width.");
        u64 value = (svalue.value() << m_rshift) & m_bitmask;
        *value_ptr = (*value_ptr & m_keep_bitmask) | value;
    }

private:
    RegisterModifier(u64 bitmask, u64 keep_bitmask, u8 rshift, ByteWidth width) : m_bitmask(bitmask), m_keep_bitmask(keep_bitmask), m_rshift(rshift), m_width(width){};

    u64 m_bitmask;
    u64 m_keep_bitmask;
    u8 m_rshift;
    ByteWidth m_width;
};
inline RegisterModifier RegisterModifier::QWORD = {0xFFFFFFFFFFFFFFFFlu, 0x0lu, 0, ByteWidth::WIDTH_QWORD};
inline RegisterModifier RegisterModifier::DWORD = {0xFFFFFFFFlu, 0x0lu, 0, ByteWidth::WIDTH_DWORD};
inline RegisterModifier RegisterModifier::WORD = {0xFFFFlu, ~0xFFFFlu, 0, ByteWidth::WIDTH_WORD};
inline RegisterModifier RegisterModifier::LOW = {0xFFlu, ~0xFFlu, 0, ByteWidth::WIDTH_BYTE};
inline RegisterModifier RegisterModifier::HIGH = {0xFF00lu, ~0xFF00lu, 8, ByteWidth::WIDTH_BYTE};




struct RegisterCallbacks {
    InterruptRaisedOr<void> (*before_read)(void*) = nullptr;
    InterruptRaisedOr<void> (*before_write)(void*) = nullptr;
    InterruptRaisedOr<void> (*after_read)(void*) = nullptr;
    InterruptRaisedOr<void> (*after_write)(void*) = nullptr;
    void* data = nullptr;

    InterruptRaisedOr<void> invoke_before_read() const {
        if (before_read != nullptr)
            return before_read(data);
        return {};
    }
    InterruptRaisedOr<void> invoke_before_write() const {
        if (before_write != nullptr)
            return before_write(data);
        return {};
    }
    InterruptRaisedOr<void> invoke_after_read() const {
        if (after_read != nullptr)
            return after_read(data);
        return {};
    }
    InterruptRaisedOr<void> invoke_after_write() const {
        if (after_write != nullptr)
            return after_write(data);
        return {};
    }
};
static RegisterCallbacks empty_callbacks = {};


enum RegisterType {
    GENERAL_PURPOSE_REGISTER,
    APPLICATION_SEGMENT_REGISTER,
    SYSTEM_SEGMENT_REGISTER,
    CONTROL_REGISTER,
    DEBUG_REGISTER,
    FLOAT_REGISTER,
    OTHER,
};

class Register {
public:
    static Register QWORD(u64* value_ptr, RegisterType type, RegisterCallbacks const& callbacks = empty_callbacks) {
        return {value_ptr, type, RegisterModifier::QWORD, callbacks};
    };
    static Register DWORD(u64* value_ptr, RegisterType type, RegisterCallbacks const& callbacks = empty_callbacks) {
        return {value_ptr, type, RegisterModifier::DWORD, callbacks};
    };
    static Register WORD(u64* value_ptr, RegisterType type, RegisterCallbacks const& callbacks = empty_callbacks) { return {value_ptr, type, RegisterModifier::WORD, callbacks}; };
    static Register LOW(u64* value_ptr, RegisterType type, RegisterCallbacks const& callbacks = empty_callbacks) { return {value_ptr, type, RegisterModifier::LOW, callbacks}; };
    static Register HIGH(u64* value_ptr, RegisterType type, RegisterCallbacks const& callbacks = empty_callbacks) { return {value_ptr, type, RegisterModifier::HIGH, callbacks}; };

    [[nodiscard]] InterruptRaisedOr<SizedValue> read() const {
        MAY_HAVE_RAISED(m_callbacks.invoke_before_read());
        auto value = m_modifier.read(m_value_ptr);
        MAY_HAVE_RAISED(m_callbacks.invoke_after_read());
        return value;
    }
    [[nodiscard]] InterruptRaisedOr<void> write(SizedValue const& svalue) {
        MAY_HAVE_RAISED(m_callbacks.invoke_before_write());
        m_modifier.write(m_value_ptr, svalue);
        MAY_HAVE_RAISED(m_callbacks.invoke_after_write());
        return {};
    }
    RegisterType type() const { return m_type; }

protected:
    Register(u64* value_ptr, RegisterType type, RegisterModifier& modifier, RegisterCallbacks const& callbacks)
        : m_value_ptr(value_ptr), m_type(type), m_modifier(modifier), m_callbacks(callbacks) {
        CPUE_ASSERT(value_ptr != nullptr, "value_ptr must not be null.");
    };

    u64* m_value_ptr;
    RegisterType m_type;
    RegisterModifier& m_modifier;
    RegisterCallbacks m_callbacks;
};

class GeneralPurposeRegister final : Register {
public:
    static GeneralPurposeRegister QWORD(u64* value_ptr, RegisterCallbacks const& callbacks = empty_callbacks) { return {value_ptr, RegisterModifier::QWORD, callbacks}; };
    static GeneralPurposeRegister DWORD(u64* value_ptr, RegisterCallbacks const& callbacks = empty_callbacks) { return {value_ptr, RegisterModifier::DWORD, callbacks}; };
    static GeneralPurposeRegister WORD(u64* value_ptr, RegisterCallbacks const& callbacks = empty_callbacks) { return {value_ptr, RegisterModifier::WORD, callbacks}; };
    static GeneralPurposeRegister LOW(u64* value_ptr, RegisterCallbacks const& callbacks = empty_callbacks) { return {value_ptr, RegisterModifier::LOW, callbacks}; };
    static GeneralPurposeRegister HIGH(u64* value_ptr, RegisterCallbacks const& callbacks = empty_callbacks) { return {value_ptr, RegisterModifier::HIGH, callbacks}; };

private:
    GeneralPurposeRegister(u64* value_ptr, RegisterModifier& modifier, RegisterCallbacks const& callbacks)
        : Register(value_ptr, RegisterType::GENERAL_PURPOSE_REGISTER, modifier, callbacks){};
};




static RegisterType _register_type_lookup_table[] = {OTHER, GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER,
    GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER,
    GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER,
    GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER,
    GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER, OTHER, OTHER, OTHER, APPLICATION_SEGMENT_REGISTER, GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER, OTHER,
    APPLICATION_SEGMENT_REGISTER, APPLICATION_SEGMENT_REGISTER, OTHER, GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER,
    GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER, OTHER, OTHER, GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER,
    GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER, APPLICATION_SEGMENT_REGISTER, CONTROL_REGISTER, CONTROL_REGISTER, CONTROL_REGISTER, CONTROL_REGISTER, CONTROL_REGISTER,
    CONTROL_REGISTER, CONTROL_REGISTER, CONTROL_REGISTER, CONTROL_REGISTER, CONTROL_REGISTER, CONTROL_REGISTER, CONTROL_REGISTER, CONTROL_REGISTER, CONTROL_REGISTER,
    CONTROL_REGISTER, CONTROL_REGISTER, DEBUG_REGISTER, DEBUG_REGISTER, DEBUG_REGISTER, DEBUG_REGISTER, DEBUG_REGISTER, DEBUG_REGISTER, DEBUG_REGISTER, DEBUG_REGISTER,
    DEBUG_REGISTER, DEBUG_REGISTER, DEBUG_REGISTER, DEBUG_REGISTER, DEBUG_REGISTER, DEBUG_REGISTER, DEBUG_REGISTER, DEBUG_REGISTER, FLOAT_REGISTER, FLOAT_REGISTER, FLOAT_REGISTER,
    FLOAT_REGISTER, FLOAT_REGISTER, FLOAT_REGISTER, FLOAT_REGISTER, FLOAT_REGISTER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER,
    OTHER, OTHER, OTHER, GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER,
    GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER,
    OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER,
    OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER,
    OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER,
    OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, OTHER, GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER,
    GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER,
    GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER,
    GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER,
    GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER, GENERAL_PURPOSE_REGISTER, OTHER, OTHER, OTHER, OTHER, OTHER};

inline RegisterType get_register_type(x86_reg reg) {
    return _register_type_lookup_table[reg];
}

}