#pragma once

#include <elfio/elfio.hpp>
#include <string>
#include <utility>
#include <vector>

#include "loader.h"

namespace CPUE {


class ELF {
public:
    explicit ELF(std::string const& filename) : m_filename(filename){};
    ELF(ELF const& other) = default;

    bool is_elf64() {
        lazy_load();
        return m_reader.get_class() == ELFIO::ELFCLASS64;
    }
    bool is_pie() {
        lazy_load();
        return m_reader.get_type() == ELFIO::ET_DYN;
    }
    bool is_executable() {
        lazy_load();
        return m_reader.get_type() == ELFIO::ET_EXEC;
    }
    bool is_static() {
        lazy_load();
        for (auto& psec : m_reader.sections) {
            if (psec->get_type() == ELFIO::PT_DYNAMIC)
                return true;
        }
        return false;
    }
    u64 entry_point() {
        lazy_load();
        return m_reader.get_entry();
    }
    std::pair<u64, u64> vas_range() {
        lazy_load_regions();
        return m_vas_range;
    }
    u64 base_address() { return vas_range().first; }

    friend class Regions;
    class Regions {
    public:
        using ValueType = Region;
        using VectorType = std::vector<ValueType>;

        explicit Regions(ELF* parent) : m_parent(parent){};

        u64 size() const { return m_parent->m_regions.size(); }
        ValueType operator[](unsigned int index) const { return m_parent->m_regions[index]; }

        VectorType::iterator begin() { return m_parent->m_regions.begin(); }
        VectorType::iterator end() { return m_parent->m_regions.end(); }
        VectorType::const_iterator begin() const { return m_parent->m_regions.cbegin(); }
        VectorType::const_iterator end() const { return m_parent->m_regions.cend(); }

    private:
        ELF* m_parent;
    };
    Regions regions() {
        lazy_load_regions();
        return Regions(this);
    }

private:
    void lazy_load_regions();
    void lazy_load();
    u64 get_region_flags_for_segment(ELFIO::segment& segment) const;

private:
    std::string m_filename;
    ELFIO::elfio m_reader;
    Regions::VectorType m_regions;
    std::pair<u64, u64> m_vas_range = std::make_pair(0, 0);
    bool m_regions_loaded = false;
    bool m_is_loaded = false;
};


}