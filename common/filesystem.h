#pragma once

#include <filesystem>
#include <span>
#include <string>

namespace filesystem
{
    void set_current_path(std::span<const char*> args);

    const std::filesystem::path& data_path();
    const std::filesystem::path& logs_path();

    bool create(const std::filesystem::path& path);
    bool remove(const std::filesystem::path& path);
    bool remove_directory(const std::filesystem::path& path);

    int file_size(const std::filesystem::path& path);
    std::string read_file(const std::filesystem::path& path, std::size_t offset, std::size_t length);
    std::string read_file(const std::filesystem::path& path);
    void write_file(const std::filesystem::path& path, const std::string& content);
}
