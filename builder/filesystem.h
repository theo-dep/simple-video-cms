#pragma once

#include <filesystem>

namespace filesystem
{
    const std::filesystem::path& data_path() noexcept;
    const std::filesystem::path& logs_path() noexcept;

    bool create(const std::filesystem::path& path) noexcept;
    bool remove(const std::filesystem::path& path) noexcept;
}
