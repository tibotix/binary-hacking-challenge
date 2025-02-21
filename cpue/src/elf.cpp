#include "elf.h"

#include "logging.h"

namespace CPUE {


std::optional<u64> ELF::find_symbol_address(std::string const& name) {
    lazy_load();

    for (const auto& section : m_reader.sections) {
        if (section->get_type() == ELFIO::SHT_SYMTAB || section->get_type() == ELFIO::SHT_DYNSYM) {
            ELFIO::symbol_section_accessor symbols(m_reader, section.get());
            std::string sym_name;
            ELFIO::Elf64_Addr value;
            ELFIO::Elf_Xword size;
            u8 bind, type, other;
            ELFIO::Elf_Half section_index;
            for (auto i = 0; i < symbols.get_symbols_num(); ++i) {
                if (symbols.get_symbol(i, sym_name, value, size, bind, type, section_index, other)) {
                    if (sym_name == name)
                        return value;
                }
            }
        }
    }
    return std::nullopt;
}


void ELF::lazy_load_regions() {
    lazy_load();
    if (m_regions_loaded)
        return;

    m_regions.reserve(m_reader.segments.size());
    for (auto& pseg : m_reader.segments) {
        // only load PT_LOAD segments into memory
        if (pseg->get_type() != ELFIO::PT_LOAD)
            continue;
        // Non-readable segments are simply not mapped
        if (!(pseg->get_flags() & ELFIO::PF_R))
            continue;
        // MemSize == 0 segments don't take space in memory -> we don't map them
        if (pseg->get_memory_size() == 0)
            continue;

        auto align = pseg->get_align();
        auto vaddr = pseg->get_virtual_address();
        auto offset = pseg->get_offset();
        CPUE_ASSERT(align == 0 || offset % align == vaddr % align, "ELF loader: unaligned segment in ELF file. We currently don't support this.");

        Region region = {
            .base = pseg->get_virtual_address(),
            .size = pseg->get_memory_size(),
            .flags = get_region_flags_for_segment(*pseg),
            .data = (u8*)pseg->get_data(),
            .data_size = pseg->get_file_size(),
        };
        if (region.base > m_vas_range.first)
            m_vas_range.first = region.base.addr;
        if (region.base + region.size > m_vas_range.second)
            m_vas_range.second = (region.base + region.size).addr;
        m_regions.push_back(region);
    }

    m_regions_loaded = true;
}

void ELF::lazy_load() {
    if (m_is_loaded)
        return;
    if (!m_reader.load(m_filename))
        fail("Error while trying to load ELF file");
    m_is_loaded = true;
}

u64 ELF::get_region_flags_for_segment(ELFIO::segment& segment) const {
    u64 flags = 0x0;
    u64 segment_flags = segment.get_flags();
    if (segment_flags & ELFIO::PF_X)
        flags |= REGION_EXECUTABLE;
    if (segment_flags & ELFIO::PF_W)
        flags |= REGION_WRITABLE;
    return flags;
}


}