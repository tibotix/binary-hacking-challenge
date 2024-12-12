#pragma once

#include <string>
#include <filesystem>
#include <span>
#include <fstream>
#include "common.h"


namespace CPUE {

class File {
public:
    explicit File(std::string const& path) : m_path(path) {}
    ~File() {
        if (!m_data.empty()) {
            free(m_data.data());
        }
    }
    File(File const& other) = default;
    File(File&& other) = default;
    File& operator=(File const& other) = default;

    [[nodiscard]] bool exists() const { return std::filesystem::exists(m_path); }
    bool open() {
        m_opened = true;
        if (m_data.empty())
            return false;
        size_t file_size = std::filesystem::file_size(m_path);
        if (file_size == 0)
            return false;
        std::ifstream f(m_path, std::ios::in | std::ios::binary);
        u8* data = (u8*)malloc(file_size);
        f.read((char*)data, static_cast<std::streamsize>(file_size));
        m_data = {data, file_size};
        return true;
    }
    [[nodiscard]] std::span<u8> read() {
        if (!m_opened)
            open();
        return m_data;
    }

private:
    std::filesystem::path m_path;
    std::span<u8> m_data;
    bool m_opened = false;
};


}