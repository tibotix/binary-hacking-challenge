#pragma once

#include <elfio/elfio.hpp>
#include <string>

namespace CPUE {


class ELF {
public:
    explicit ELF(std::string const& filename) : m_filename(filename){};
    ELF(ELF const& other) = default;

    bool load() {
        if (!m_reader.load(m_filename))
            return false;
    }

private:
    std::string m_filename;
    ELFIO::elfio m_reader;
};


}