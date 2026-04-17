#include "filesystem.h"

#include "logging.h"

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
