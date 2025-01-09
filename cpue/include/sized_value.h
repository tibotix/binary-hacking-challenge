#pragma once

#include "common.h"

namespace CPUE {

class SizedValue {
public:
    SizedValue() = default;
    template<unsigned_integral T>
    explicit SizedValue(T value) : m_value(value), m_width(get_byte_width<T>()) {}
    SizedValue(u64 value, ByteWidth width) : m_value(value & bitmask64(width)), m_width(width) {}

    u64 value() const { return m_value; }

    template<unsigned_integral T>
    T value_as() const {
        return static_cast<T>(value());
    }

    ByteWidth width() const { return m_width; }

private:
    u64 m_value;
    ByteWidth m_width;
};

}