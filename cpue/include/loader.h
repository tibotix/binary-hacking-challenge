#pragma once

#include <cmath>

#include "common.h"
#include "address.h"
#include "cpu.h"

namespace CPUE {

struct Region {
    VirtualAddress base;
    u64 size;
    u8* data = nullptr;

    u64 get_page_count() const { return std::ceil(size / PAGE_SIZE); }
};

class Loader {
public:
    Loader(CPU& cpu) : m_cpu(cpu) {}

    void load_region(Region const& region, u64& top);
    /**
     *
     */
    void create_region_vas(Region const& region, u64& top);

private:
    CPU& m_cpu;
};

}