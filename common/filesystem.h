#pragma once

#include <filesystem>
#include <span>

namespace filesystem
{
    void set_current_path(std::span<const char*> args);

    const std::filesystem::path& data_path();
    const std::filesystem::path& logs_path();

    bool create(const std::filesystem::path& path);
    bool remove(const std::filesystem::path& path);
    bool remove_directory(const std::filesystem::path& path);
}
