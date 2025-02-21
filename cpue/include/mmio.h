#pragma once

#include <concepts>
#include <optional>
#include <set>
#include <utility>

#include "common.h"
#include "address.h"
#include "interrupts.h"
#include "endianness.h"


namespace CPUE {


typedef SizedValue (*mmio_reg_read_t)(void*, u64);
typedef void (*mmio_reg_write_t)(void*, SizedValue const&, u64);

struct MMIOReg {
    const ByteWidth width = ByteWidth::WIDTH_BYTE;
    mmio_reg_read_t read_func = nullptr;
    mmio_reg_write_t write_func = nullptr;
    void* data = nullptr;

    SizedValue read(u64 offset_in_bytes = 0x0) const {
        CPUE_ASSERT(read_func != nullptr, "read_func is null");
        return read_func(data, offset_in_bytes) & bytemask(width);
    }
    void write(SizedValue value, u64 offset_in_bytes = 0x0) const {
        CPUE_ASSERT(write_func != nullptr, "read_func is null");
        return write_func(data, value & bytemask(width), offset_in_bytes);
    }
};

template<unsigned_integral T>
MMIOReg make_ptr_mmio_reg(T* ptr) {
    CPUE_ASSERT(ptr != nullptr, "ptr is null");
    return MMIOReg{
        .width = get_byte_width<T>(),
        .read_func = [](void* data, u64) -> SizedValue {
            return SizedValue(*static_cast<T*>(data));
        },
        .write_func =
            [](void* data, SizedValue const& value, u64) {
                *static_cast<T*>(data) = static_cast<T>(value.value());
            },
        .data = ptr,
    };
}


class MMIO {
public:
    MMIO() = default;

    void map_mmio_register(PhysicalAddress const& paddr, MMIOReg const& reg);

    template<unsigned_integral T>
    [[nodiscard]] InterruptRaisedOr<std::optional<BigEndian<T>>> try_mmio_read(PhysicalAddress const& paddr) {
        auto [it, offset_in_bytes] = find_mmio_register(paddr);
        if (it == m_mmio_regs.end())
            return std::nullopt;
        return it->reg.read(offset_in_bytes).as<T>();
    }
    template<unsigned_integral T>
    [[nodiscard]] InterruptRaisedOr<bool> try_mmio_write(PhysicalAddress const& paddr, BigEndian<T> const& value) {
        auto [it, offset_in_bytes] = find_mmio_register(paddr);
        if (it == m_mmio_regs.end())
            return false;
        it->reg.write(SizedValue(value.value), offset_in_bytes);
        return true;
    }

private:
    struct MappedMMIOReg {
        PhysicalAddress base;
        MMIOReg reg;

        bool operator<(MappedMMIOReg const& other) const { return base < other.base; }
    };

    std::pair<std::set<MappedMMIOReg>::iterator, u8> find_mmio_register(PhysicalAddress const& paddr);

    std::set<MappedMMIOReg> m_mmio_regs;
};


}