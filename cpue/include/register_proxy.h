#pragma once

#include "common.h"
#include "segment_register.h"
#include "sized_value.h"
#include "interrupts.h"
#include "forward.h"

namespace CPUE {


class RegisterValueModifier {
public:
    static RegisterValueModifier QWORD;
    static RegisterValueModifier DWORD;
    static RegisterValueModifier WORD;
    static RegisterValueModifier LOW;
    static RegisterValueModifier HIGH;

    [[nodiscard]] SizedValue read(u64 const* value_ptr) const { return {(*value_ptr & m_bitmask) >> m_rshift, m_width}; }
    void write(u64* value_ptr, SizedValue const& svalue) {
        CPUE_ASSERT(svalue.byte_width() <= m_width, "Invalid width.");
        u64 value = (svalue << m_rshift) & m_bitmask;
        *value_ptr = (*value_ptr & m_keep_bitmask) | value;
    }

    ByteWidth byte_width() const { return m_width; }

private:
    RegisterValueModifier(u64 bitmask, u64 keep_bitmask, u8 rshift, ByteWidth width) : m_bitmask(bitmask), m_keep_bitmask(keep_bitmask), m_rshift(rshift), m_width(width){};

    u64 m_bitmask;
    u64 m_keep_bitmask;
    u8 m_rshift;
    ByteWidth m_width;
};
inline RegisterValueModifier RegisterValueModifier::QWORD = {0xFFFFFFFFFFFFFFFFlu, 0x0lu, 0, ByteWidth::WIDTH_QWORD};
inline RegisterValueModifier RegisterValueModifier::DWORD = {0xFFFFFFFFlu, 0x0lu, 0, ByteWidth::WIDTH_DWORD};
inline RegisterValueModifier RegisterValueModifier::WORD = {0xFFFFlu, ~0xFFFFlu, 0, ByteWidth::WIDTH_WORD};
inline RegisterValueModifier RegisterValueModifier::LOW = {0xFFlu, ~0xFFlu, 0, ByteWidth::WIDTH_BYTE};
inline RegisterValueModifier RegisterValueModifier::HIGH = {0xFF00lu, ~0xFF00lu, 8, ByteWidth::WIDTH_BYTE};




struct RegisterCallbacks {
    InterruptRaisedOr<void> (*before_read)(void*) = nullptr;
    InterruptRaisedOr<void> (*after_read)(void*) = nullptr;
    InterruptRaisedOr<void> (*before_write)(void*) = nullptr;
    InterruptRaisedOr<void> (*after_write)(void*) = nullptr;
    void* data = nullptr;

    InterruptRaisedOr<void> invoke_before_read() const { return invoke<InterruptRaisedOr<void>, decltype(before_read), void* const>(before_read, this->data); }
    InterruptRaisedOr<void> invoke_before_write() const { return invoke<InterruptRaisedOr<void>, decltype(before_write), void* const>(before_write, this->data); }
    InterruptRaisedOr<void> invoke_after_read() const { return invoke<InterruptRaisedOr<void>, decltype(after_read), void* const>(after_read, this->data); }
    InterruptRaisedOr<void> invoke_after_write() const { return invoke<InterruptRaisedOr<void>, decltype(after_write), void* const>(after_write, this->data); }

private:
    template<typename T, typename Func, typename... Args>
    requires std::is_same_v<T, std::invoke_result_t<Func, Args...>> T invoke(Func f, Args&... args)
    const {
        if (f != nullptr)
            return f(std::forward<Args>(args)...);
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
enum RegisterAccessFlags : u8 {
    REG_ACCESS_NONE = 0,
    REG_ACCESS_READ = 1,
    REG_ACCESS_WRITE = 2,
};
class RegisterProxy {
public:
    RegisterProxy(CPU* cpu, u8 flags, ByteWidth width, RegisterCallbacks const& callbacks) : m_cpu(cpu), m_flags(flags), m_width(width), m_callbacks(callbacks){};
    virtual ~RegisterProxy() = default;

    virtual InterruptRaisedOr<SizedValue> read() const {
        MAY_HAVE_RAISED(before_read());
        auto value = MAY_HAVE_RAISED(do_read());
        MAY_HAVE_RAISED(after_read());
        return value;
    }
    virtual InterruptRaisedOr<void> write(SizedValue const& value) {
        MAY_HAVE_RAISED(before_write());
        MAY_HAVE_RAISED(do_write(value));
        MAY_HAVE_RAISED(after_write());
        return {};
    }

    virtual RegisterType type() const = 0;
    ByteWidth byte_width() const { return m_width; }

protected:
    virtual InterruptRaisedOr<void> before_read() const;
    virtual InterruptRaisedOr<SizedValue> do_read() const = 0;
    virtual InterruptRaisedOr<void> after_read() const { return {}; }
    virtual InterruptRaisedOr<void> before_write() const;
    virtual InterruptRaisedOr<void> do_write(SizedValue const&) = 0;
    virtual InterruptRaisedOr<void> after_write() const { return {}; }

    CPU* m_cpu;
    u8 m_flags;
    ByteWidth m_width;
    RegisterCallbacks m_callbacks;
};

class GeneralPurposeRegisterProxy final : public RegisterProxy {
public:
    static GeneralPurposeRegisterProxy QWORD(u64* value_ptr, CPU* cpu, u8 flags = REG_ACCESS_READ | REG_ACCESS_WRITE, RegisterCallbacks const& callbacks = empty_callbacks) {
        return {value_ptr, RegisterValueModifier::QWORD, cpu, flags, callbacks};
    };
    static GeneralPurposeRegisterProxy DWORD(u64* value_ptr, CPU* cpu, u8 flags = REG_ACCESS_READ | REG_ACCESS_WRITE, RegisterCallbacks const& callbacks = empty_callbacks) {
        return {value_ptr, RegisterValueModifier::DWORD, cpu, flags, callbacks};
    };
    static GeneralPurposeRegisterProxy WORD(u64* value_ptr, CPU* cpu, u8 flags = REG_ACCESS_READ | REG_ACCESS_WRITE, RegisterCallbacks const& callbacks = empty_callbacks) {
        return {value_ptr, RegisterValueModifier::WORD, cpu, flags, callbacks};
    };
    static GeneralPurposeRegisterProxy LOW(u64* value_ptr, CPU* cpu, u8 flags = REG_ACCESS_READ | REG_ACCESS_WRITE, RegisterCallbacks const& callbacks = empty_callbacks) {
        return {value_ptr, RegisterValueModifier::LOW, cpu, flags, callbacks};
    };
    static GeneralPurposeRegisterProxy HIGH(u64* value_ptr, CPU* cpu, u8 flags = REG_ACCESS_READ | REG_ACCESS_WRITE, RegisterCallbacks const& callbacks = empty_callbacks) {
        return {value_ptr, RegisterValueModifier::HIGH, cpu, flags, callbacks};
    };

    RegisterType type() const override { return GENERAL_PURPOSE_REGISTER; }

protected:
    InterruptRaisedOr<SizedValue> do_read() const override;
    InterruptRaisedOr<void> do_write(SizedValue const&) override;

private:
    GeneralPurposeRegisterProxy(u64* value_ptr, RegisterValueModifier const& value_modifier, CPU* cpu, u8 flags, RegisterCallbacks const& callbacks)
        : RegisterProxy(cpu, flags, value_modifier.byte_width(), callbacks), m_value_ptr(value_ptr), m_value_modifier(value_modifier) {}

    u64* m_value_ptr;
    RegisterValueModifier m_value_modifier;
};


class ControlRegisterProxy final : public RegisterProxy {
public:
    ControlRegisterProxy(u64* value_ptr, CPU* cpu, u8 flags = REG_ACCESS_READ | REG_ACCESS_WRITE, RegisterCallbacks const& callbacks = empty_callbacks)
        : RegisterProxy(cpu, flags, ByteWidth::WIDTH_QWORD, callbacks), m_value_ptr(value_ptr) {}

    RegisterType type() const override { return CONTROL_REGISTER; }

protected:
    InterruptRaisedOr<SizedValue> do_read() const override;
    InterruptRaisedOr<void> do_write(SizedValue const&) override;

private:
    u64* m_value_ptr;
};

class ApplicationSegmentRegisterProxy final : public RegisterProxy {
public:
    ApplicationSegmentRegisterProxy(ApplicationSegmentRegister* seg_ptr, SegmentRegisterAlias alias, CPU* cpu, u8 flags = REG_ACCESS_READ | REG_ACCESS_WRITE,
        RegisterCallbacks const& callbacks = empty_callbacks)
        : RegisterProxy(cpu, flags, ByteWidth::WIDTH_WORD, callbacks), m_seg_ptr(seg_ptr), m_seg_alias(alias) {}

    RegisterType type() const override { return APPLICATION_SEGMENT_REGISTER; }

protected:
    InterruptRaisedOr<SizedValue> do_read() const override;
    InterruptRaisedOr<void> do_write(SizedValue const&) override;

private:
    ApplicationSegmentRegister* m_seg_ptr;
    SegmentRegisterAlias m_seg_alias;
};



}