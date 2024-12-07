#pragma once

#include <optional>
#include "common.h"

namespace CPUE {

template<typename T, u64 cap>
class InplaceFIFO {
public:
    InplaceFIFO() = default;
    InplaceFIFO(std::array<T, cap>&& initial) : m_data{std::move(initial)} {};

    bool enqueue(T const& data) {
        if (size() == m_data.size())
            return false;
        m_data[inc_tail()] = data;
        return true;
    }
    std::optional<T> take_first() {
        if (m_head == m_tail)
            return std::nullopt;
        return m_data[inc_head()];
    }

    void clear() { m_head = m_tail = 0; }
    u64 size() const { return (((m_tail - m_head) % cap) + cap) % cap; }
    bool is_empty() const { return size() == 0; }
    bool is_full() const { return size() == cap; }
    u64 capacity_left() const { return cap - size(); }

private:
    u64 inc_head() {
        m_head = (m_head + 1) % cap;
        return m_head;
    }
    u64 inc_tail() {
        m_tail = (m_tail + 1) % cap;
        return m_tail;
    }

    std::array<T, cap> m_data;
    u64 m_head = 0;
    u64 m_tail = 0;
};

};