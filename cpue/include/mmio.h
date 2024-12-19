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


typedef BigEndian<u64> (*mmio_reg_read_t)(void*);
typedef void (*mmio_reg_write_t)(void*, BigEndian<u64>);

struct MMIOReg {
    const ByteWidth width = ByteWidth::WIDTH_BYTE;
    mmio_reg_read_t read_func = nullptr;
    mmio_reg_write_t write_func = nullptr;
    void* data = nullptr;

    BigEndian<u64> read() const {
        CPUE_ASSERT(read_func != nullptr, "read_func is null");
        return read_func(data) & bitmask64(8 * width);
    }
    void write(BigEndian<u64> value) const {
        CPUE_ASSERT(write_func != nullptr, "read_func is null");
        return write_func(data, value & bitmask64(8 * width));
    }
};

template<unsigned_integral T>
MMIOReg make_ptr_mmio_reg(T* ptr) {
    CPUE_ASSERT(ptr != nullptr, "ptr is null");
    return MMIOReg{
        .width = get_byte_width<T>(),
        .read_func = [](void* data) -> BigEndian<u64> {
            return *static_cast<u64*>(data);
        },
        .write_func =
            [](void* data, BigEndian<u64> value) {
                *static_cast<T*>(data) = static_cast<T>(value);
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
        auto offset_in_bits = offset_in_bytes * 8;
        // NOTE: we support mismatched register and read width.
        // Example: reading 2 bytes from base addr of 4byte register with value 0x01020304 (stored 0x04030201 in mem) results in 0x0304
        // Example: reading 8 bytes from base addr of 4byte register with value 0x01020304 (stored 0x04030201 in mem) results in 0x0000000001020304
        return (it->reg.read() >> offset_in_bits) & bitmask64(8 * get_byte_width<T>());
    }
    template<unsigned_integral T>
    [[nodiscard]] InterruptRaisedOr<bool> try_mmio_write(PhysicalAddress const& paddr, BigEndian<T> const& value) {
        auto [it, offset_in_bytes] = find_mmio_register(paddr);
        if (it == m_mmio_regs.end())
            return false;
        auto offset_in_bits = offset_in_bytes * 8;
        // NOTE: we support mismatched register and write width.
        // Example: writing 0x0a0b (0x0b0a in le) to base addr of 4byte register with value 0x01020304 (stored 0x04030201 in mem) results in 0x01020a0b
        // Example: writing 0x0a0b0c0d (0x0d0c0b0a in le) to base addr of 2byte register with value 0x0102 (stored 0x0201 in mem) results in 0x0c0d
        auto prev = it->reg.read();
        auto offset_val = prev & bitmask64(offset_in_bits);
        u64 val = (((u64)value << offset_in_bits) | offset_val) & bitmask64(8 * it->reg.width);
        if (get_byte_width<T>() + offset_in_bytes < it->reg.width) {
            val = (prev & ~bitmask64(8 * it->reg.width - offset_in_bits)) | val;
        }
        it->reg.write(val);
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