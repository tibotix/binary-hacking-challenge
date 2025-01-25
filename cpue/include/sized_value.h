#pragma once

#include "common.h"

namespace CPUE {

class SizedValue {
public:
    SizedValue() = default;
    template<unsigned_integral T>
    constexpr explicit SizedValue(T value) : m_value(value), m_width(get_byte_width<T>()) {}
    constexpr SizedValue(u64 value, ByteWidth width) : m_value(value & bytemask(width)), m_width(width) {}

    constexpr SizedValue& operator=(u64 value) {
        set(value);
        return *this;
    }
    constexpr SizedValue& operator=(SizedValue const& value) = default;

    constexpr SizedValue& operator+=(u64 value) {
        set(m_value + value);
        return *this;
    }
    constexpr SizedValue& operator+=(SizedValue const& value) { return operator+=(value.m_value); }
    constexpr SizedValue& operator-=(u64 value) {
        set(m_value - value);
        return *this;
    }
    constexpr SizedValue& operator-=(SizedValue const& value) { return operator-=(value.m_value); }

    constexpr SizedValue operator+(u64 value) { return {m_value + value, m_width}; }
    constexpr SizedValue operator-(u64 value) { return {m_value - value, m_width}; }
    constexpr SizedValue operator|(u64 value) { return {m_value | value, m_width}; }
    constexpr SizedValue operator&(u64 value) { return {m_value & value, m_width}; }
    constexpr SizedValue operator~() { return {~m_value, m_width}; }
    constexpr SizedValue operator^(u64 value) { return {m_value ^ value, m_width}; }
    constexpr SizedValue operator+(SizedValue const& value) { return operator+(value.m_value); }
    constexpr SizedValue operator-(SizedValue const& value) { return operator-(value.m_value); }
    constexpr SizedValue operator|(SizedValue const& value) { return operator|(value.m_value); }
    constexpr SizedValue operator&(SizedValue const& value) { return operator&(value.m_value); }
    constexpr SizedValue operator^(SizedValue const& value) { return operator^(value.m_value); }

    constexpr bool operator>(u64 value) const { return m_value > value; }
    constexpr bool operator>=(u64 value) const { return m_value >= value; }
    constexpr bool operator<(u64 value) const { return m_value < value; }
    constexpr bool operator<=(u64 value) const { return m_value <= value; }
    constexpr bool operator==(u64 value) const { return m_value == value; }
    constexpr bool operator==(SizedValue const& other) const { return operator==(other.m_value); }

    constexpr u64 operator>>(int pos) const { return m_value >> pos; }
    constexpr u64 operator<<(int pos) const { return m_value << pos; }


    constexpr u64 value() const { return m_value; }

    template<unsigned_integral T>
    constexpr T as() const {
        return static_cast<T>(value());
    }

    constexpr u8 sign_bit() const { return (m_value >> (bit_width() - 1)) & 1; }

    constexpr u64 max_val() const { return m_width.bitmask(); }
    constexpr ByteWidth byte_width() const { return m_width; }
    constexpr u16 bit_width() const { return byte_width() * 8; }

    template<typename T, typename Func>
    constexpr T do_with_concrete_type(Func& f) const {
        auto func = [&]<typename U>() -> T {
            return f(static_cast<U>(m_value));
        };
        return m_width.do_with_concrete_type<T, decltype(func)>(func);
    }

private:
    constexpr void set(u64 value) { m_value = value & bytemask(m_width); }

    u64 m_value;
    ByteWidth m_width;
};

}
