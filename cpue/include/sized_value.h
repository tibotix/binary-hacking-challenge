#pragma once

#include "common.h"
#include <limits>

namespace CPUE {


class SizedValue {
public:
    SizedValue() = default;
    template<integral T>
    constexpr explicit SizedValue(T value) : m_value(value), m_width(get_byte_width<T>()) {}
    constexpr SizedValue(u128 value, ByteWidth width) : m_value(value & bytemask(width)), m_width(width) {}

    constexpr SizedValue& operator--() { return operator-=(1); }
    constexpr SizedValue operator--(int) {
        auto cpy = *this;
        --*this;
        return cpy;
    }

    constexpr SizedValue& operator++() { return operator+=(1); }
    constexpr SizedValue operator++(int) {
        auto cpy = *this;
        ++*this;
        return cpy;
    }

    constexpr SizedValue& operator=(u128 value) {
        set(value);
        return *this;
    }
    constexpr SizedValue& operator=(SizedValue const& value) = default;

    constexpr SizedValue& operator+=(u128 value) {
        set(m_value + value);
        return *this;
    }
    constexpr SizedValue& operator+=(SizedValue const& value) { return operator+=(value.m_value); }
    constexpr SizedValue& operator-=(u128 value) {
        set(m_value - value);
        return *this;
    }
    constexpr SizedValue& operator-=(SizedValue const& value) { return operator-=(value.m_value); }

    constexpr SizedValue operator+(u128 value) const { return {m_value + value, m_width}; }
    constexpr SizedValue operator-(u128 value) const { return {m_value - value, m_width}; }
    constexpr SizedValue operator/(u128 value) const { return {m_value / value, m_width}; }
    constexpr SizedValue operator*(u128 value) const { return {m_value * value, m_width}; }
    constexpr SizedValue operator%(u128 value) const { return {m_value % value, m_width}; }
    constexpr SizedValue operator|(u128 value) const { return {m_value | value, m_width}; }
    constexpr SizedValue operator&(u128 value) const { return {m_value & value, m_width}; }
    constexpr SizedValue operator~() const { return {~m_value, m_width}; }
    constexpr SizedValue operator^(u128 value) const { return {m_value ^ value, m_width}; }
    constexpr SizedValue operator+(SizedValue const& value) const { return operator+(value.m_value); }
    constexpr SizedValue operator-(SizedValue const& value) const { return operator-(value.m_value); }
    constexpr SizedValue operator/(SizedValue const& value) const { return operator/(value.m_value); }
    constexpr SizedValue operator*(SizedValue const& value) const { return operator*(value.m_value); }
    constexpr SizedValue operator%(SizedValue const& value) const { return operator%(value.m_value); }
    constexpr SizedValue operator|(SizedValue const& value) const { return operator|(value.m_value); }
    constexpr SizedValue operator&(SizedValue const& value) const { return operator&(value.m_value); }
    constexpr SizedValue operator^(SizedValue const& value) const { return operator^(value.m_value); }
    constexpr SizedValue operator>>(int pos) const { return {m_value >> pos, m_width}; }
    constexpr SizedValue operator<<(int pos) const { return {m_value << pos, m_width}; }

    constexpr bool operator>(u128 value) const { return m_value > value; }
    constexpr bool operator>=(u128 value) const { return m_value >= value; }
    constexpr bool operator<(u128 value) const { return m_value < value; }
    constexpr bool operator<=(u128 value) const { return m_value <= value; }
    constexpr bool operator==(u128 value) const { return m_value == value; }
    constexpr bool operator==(SizedValue const& other) const { return operator==(other.m_value); }


    [[nodiscard]] constexpr u128 value() const { return m_value; }
    [[nodiscard]] constexpr i128 signed_value() const { return as_signed<i128>(); }
    constexpr SizedValue upper_half() const { return {m_value >> m_width.half_width().bit_width(), m_width.half_width()}; }
    constexpr SizedValue lower_half() const { return {m_value & m_width.half_width().bitmask(), m_width.half_width()}; }

    template<typename T>
    constexpr T as() const {
        return static_cast<T>(m_value);
    }

    template<unsigned_integral T>
    constexpr T as_unsigned() const {
        return static_cast<T>(m_value);
    }
    template<signed_integral T>
    constexpr T as_signed() const {
        if (m_value > std::numeric_limits<decltype(m_value)>::max()) {
            return -static_cast<T>(~m_value) - 1;
        }
        return static_cast<T>(m_value);
    }

    constexpr u8 sign_bit() const { return (m_value >> (bit_width() - 1)) & 1; }
    constexpr u8 msb() const { return sign_bit(); }
    constexpr u8 lsb() const { return m_value & 1; }


    [[nodiscard]] constexpr SizedValue zero_extended_or_truncated_to_width(ByteWidth width) const { return {m_value, width}; }
    [[nodiscard]] constexpr SizedValue zero_extended_to_width(ByteWidth width) const { return {m_value, width}; }
    [[nodiscard]] constexpr SizedValue sign_extended_to_width(ByteWidth width) const {
        if (m_width >= ByteWidth::WIDTH_DQWORD)
            return {m_value, width};
        if (sign_bit())
            return {(width.bitmask() << bit_width()) | m_value, width};
        return {m_value, width};
    }
    [[nodiscard]] constexpr SizedValue truncated_to_width(ByteWidth width) const { return {m_value, width}; }



    constexpr u128 max_val() const { return m_width.bitmask(); }
    constexpr ByteWidth byte_width() const { return m_width; }
    constexpr u16 bit_width() const { return m_width.bit_width(); }

    template<typename T, typename Func>
    constexpr T do_with_concrete_type(Func& f) const {
        auto func = [&]<typename U>() -> T {
            return f(static_cast<U>(m_value));
        };
        return m_width.do_with_concrete_type<T, decltype(func)>(func);
    }

private:
    constexpr void set(u128 value) { m_value = value & bytemask(m_width); }

    u128 m_value;
    ByteWidth m_width;
};

constexpr SizedValue operator""_sv(unsigned long long bytes) {
    return SizedValue(bytes);
}

}
