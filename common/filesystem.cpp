#include "filesystem.h"

#include "logging.h"

#include <fstream>

void filesystem::set_current_path(std::span<const char*> args)
{
    std::filesystem::current_path(std::filesystem::path(args[0]).parent_path());
}

const std::filesystem::path& filesystem::data_path()
{
    static const std::filesystem::path data_path{ std::filesystem::current_path() / "data" };
    return data_path;
}

const std::filesystem::path& filesystem::logs_path()
{
    static const std::filesystem::path logs_path{ data_path() / "logs" };
    return logs_path;
}

bool filesystem::create(const std::filesystem::path& path)
{
    if (std::filesystem::exists(path))
        return true;

    std::error_code error_code;
    if (!std::filesystem::create_directories(path, error_code) || error_code) {
        logging::error{ R"(Fail to create "{}" directories: {} ({}))", path.string(), error_code.message(), error_code.value() };
        return false;
    }
    return true;
}

bool filesystem::remove(const std::filesystem::path& path)
{
    if (!std::filesystem::exists(path))
        return true;

    std::error_code error_code;
    if (!std::filesystem::remove(path, error_code) || error_code) {
        logging::error{ R"(Fail to remove "{}" directories: {} ({}))", path.string(), error_code.message(), error_code.value() };
        return false;
    }
    return true;
}

bool filesystem::remove_directory(const std::filesystem::path& path)
{
    std::error_code error_code;
    if (std::filesystem::remove_all(path, error_code) < 0 || error_code) {
        logging::error{ R"(Fail to remove "{}" directories: {} ({}))", path.string(), error_code.message(), error_code.value() };
        return false;
    }
    return true;
}

int filesystem::file_size(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::in | std::ios::binary | std::ios::ate);
    return static_cast<int>(file.tellg());
}

std::string filesystem::read_file(const std::filesystem::path& path, std::size_t offset, std::size_t length)
{
    std::ifstream file(path, std::ios::in | std::ios::binary);
    file.seekg(static_cast<std::streamoff>(offset));

    std::string file_content;
    file_content.resize_and_overwrite(length, [&file](char* buffer, std::size_t buffer_size) -> std::size_t {
        file.read(buffer, static_cast<std::streamoff>(buffer_size));
        return file.gcount();
    });
    return file_content;
}

std::string filesystem::read_file(const std::filesystem::path& path)
{
    // https://insanecoding.blogspot.com/2011/11/how-to-read-in-file-in-c.html
    std::ifstream file(path, std::ios::in | std::ios::binary | std::ios::ate);
    const std::size_t file_length{ static_cast<std::size_t>(file.tellg()) };
    return read_file(path, 0, file_length);
}

void filesystem::write_file(const std::filesystem::path& path, const std::string& content)
{
    std::ofstream file(path, std::ios::out | std::ios::binary | std::ios::trunc);
    file.write(content.data(), static_cast<std::streamoff>(content.size()));
}
