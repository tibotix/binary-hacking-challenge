#pragma once

#include <elfio/elfio.hpp>
#include <string>

namespace CPUE {


class ELF {
public:
    explicit ELF(std::string const& filename) : m_filename(filename){};
    ELF(ELF const& other) = default;

    void load() {
        if (!m_reader.load(m_filename)) {
            // failed to load elf
        }
    }

private:
    std::string m_filename;
    ELFIO::elfio m_reader;
};


}