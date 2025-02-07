#pragma once

#include <cmath>

#include "common.h"
#include "address.h"
#include "cpu.h"
#include "forward.h"

namespace CPUE {

// These are the corresponding bits in the page-table-structures.
constexpr u64 REGION_WRITABLE = 1ull << 1;
constexpr u64 REGION_USER = 1ull << 2;
constexpr u64 REGION_EXECUTABLE = 1ull << 63;

struct Region {
    VirtualAddress base;
    u64 size;
    u64 flags = REGION_WRITABLE;
    u8* data = nullptr;
    u64 data_size = 0;

    [[nodiscard]] u64 get_page_count() const { return std::ceil(static_cast<double>(size) / PAGE_SIZE); }
};

class Loader {
public:
    // TODO: maybe just take MMU, not CPU
    explicit Loader(CPU& cpu) : m_cpu(cpu) {}

    void load_user_elf(ELF* elf, u64& top) { load_elf(elf, top, true); }
    void load_supervisor_elf(ELF* elf, u64& top) { load_elf(elf, top, false); }

    /**
    * Loads the given Region [region] into VAS (Virtual-Address-Space).
    * The virtual memory region where [region] resides MUST NOT be already mapped.
    * This means overwriting any already mapped virtual memory is not allowed and will result
    * in an assertion error.
    */
    void load_region(Region const& region, u64& top);
    /**
     * This function creates the necessary page-table structures to map the Region [region]
     * into VAS (Virtual-Address-Space).
     * It is assumed that the CR3 value already points to a valid PML4 with enough space (0x1000 bytes).
     * [top] hereby denotes the physical address of the top-pointer, that is, where new physical memory allocations
     * are cut from.
     * The [initial_mapping_strategy] function is used to initialize the PTE's with an initial page frame number.
     *
     * Returns true if any new PTE was created, false otherwise.
     */
    bool create_region_vas(Region const& region, u64& top, u64 (*initial_mapping_strategy)(u64) = MappingStrategies::zero);

    struct MappingStrategies {
        static constexpr u64 zero(u64 page) { return 0x0; }
        static constexpr u64 identity(u64 page) { return page; }
    };

private:
    void load_elf(ELF* elf, u64& top, bool user_elf);

    CPU& m_cpu;
};

}