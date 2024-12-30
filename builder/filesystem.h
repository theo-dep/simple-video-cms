#pragma once

#include <filesystem>

namespace filesystem
{
    const std::filesystem::path& data_path();
    const std::filesystem::path& logs_path();

    bool create(const std::filesystem::path& path);
    bool remove(const std::filesystem::path& path);
}
