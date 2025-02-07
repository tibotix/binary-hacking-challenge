#pragma once

#include <optional>
#include <array>
#include "common.h"

namespace CPUE {

template<typename T, i64 cap>
class InplaceFIFO {
    static_assert(std::is_signed_v<decltype(cap)>, "cap must be signed for modulo to work properly.");
    static_assert(cap > 0, "capacity must be greater than 0");

public:
    InplaceFIFO() = default;
    InplaceFIFO(std::array<T, cap>&& initial) : m_data{std::move(initial)} {};

    bool try_enqueue(T const& data) {
        if (is_full())
            return false;
        m_data[inc_tail()] = data;
        return true;
    }
    std::optional<T> take_first() {
        if (is_empty())
            return std::nullopt;
        return m_data[inc_head()];
    }

    void clear() {
        m_head = m_tail = 0;
        full = false;
    }
    u64 size() const {
        i64 diff = static_cast<i64>(m_tail) - m_head;
        return (((diff % cap) + cap) % cap) + cap * full;
    }
    bool is_empty() const { return size() == 0; }
    bool is_full() const { return size() == cap; }
    u64 capacity_left() const { return cap - size(); }

private:
    u64 inc_head() {
        auto temp = m_head;
        m_head = (m_head + 1) % cap;
        full = false;
        return temp;
    }
    u64 inc_tail() {
        auto temp = m_tail;
        m_tail = (m_tail + 1) % cap;
        full = m_tail == m_head;
        return temp;
    }

    std::array<T, cap> m_data;
    u64 m_head = 0;
    u64 m_tail = 0;
    bool full = false;
};

};