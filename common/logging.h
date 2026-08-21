#pragma once

#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <source_location>
#include <string>
#include <thread>

namespace logging
{
    void init(const std::filesystem::path& log_file_path);

    std::string time_local();
    void raw_log(const std::string& message); // for server log

    // https://www.cppstories.com/2020/09/variadic-pack-first.html/
    // https://cor3ntin.github.io/posts/variadic/
    // https://en.cppreference.com/w/cpp/language/class_template_argument_deduction#User-defined_deduction_guides
    template <class... Args>
    struct info // NOLINT(readability-identifier-naming): used as a function
    {
        info(std::format_string<Args...> fmt, Args&&... args, const std::source_location& location = std::source_location::current());
    };
    template <class... Args>
    info(std::format_string<Args...> fmt, Args&&... args) -> info<Args...>;

    template <>
    struct info<std::string>
    {
        info(const std::string& message, const std::source_location& location = std::source_location::current());
    };
    info(const std::string& message) -> info<std::string>;

    template <class... Args>
    struct error // NOLINT(readability-identifier-naming): used as a function
    {
        error(std::format_string<Args...> fmt, Args&&... args, const std::source_location& location = std::source_location::current());
    };
    template <class... Args>
    error(std::format_string<Args...> fmt, Args&&... args) -> error<Args...>;

    template <>
    struct error<std::string>
    {
        error(const std::string& message, const std::source_location& location = std::source_location::current());
    };
    error(const std::string& message) -> error<std::string>;

    template <class... Args>
    struct debug // NOLINT(readability-identifier-naming): used as a function
    {
        debug(std::format_string<Args...> fmt, Args&&... args, const std::source_location& location = std::source_location::current());
    };
    template <class... Args>
    debug(std::format_string<Args...> fmt, Args&&... args) -> debug<Args...>;

    template <>
    struct debug<std::string>
    {
        debug(const std::string& message, const std::source_location& location = std::source_location::current());
    };
    debug(const std::string& message) -> debug<std::string>;

    class Logger
    {
    public:
        Logger();
        ~Logger();

        void open(const std::filesystem::path& log_file_path);

        template <typename T>
        Logger& operator<<(const T& message);

    protected:
        std::filesystem::path log_id_file_path() const;
        void flush();
        bool is_max_log_reached();

    private:
        std::filesystem::path _log_file_path;
        std::size_t _log_file_id{ 0 };
        std::filesystem::file_time_type _latest_time_check;

        std::ofstream _log_file;
        bool _is_running{ true };
        std::thread _flush_thread;

    public:
        // prevent copy/move
        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;
        Logger(Logger&&) = delete;
        Logger& operator=(Logger&&) = delete;
    };
}

template <class... Args>
logging::info<Args...>::info(std::format_string<Args...> fmt, Args&&... args, const std::source_location& location)
{
    info<std::string>{ std::format(fmt, std::forward<Args>(args)...), location };
}

template <class... Args>
logging::error<Args...>::error(std::format_string<Args...> fmt, Args&&... args, const std::source_location& location)
{
    error<std::string>{ std::format(fmt, std::forward<Args>(args)...), location };
}

template <class... Args>
logging::debug<Args...>::debug(std::format_string<Args...> fmt, Args&&... args, const std::source_location& location)
{
    debug<std::string>{ std::format(fmt, std::forward<Args>(args)...), location };
}

template <typename T>
logging::Logger& logging::Logger::operator<<(const T& message)
{
    _log_file << message; // NOLINT(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
    std::clog << message; // NOLINT(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
    return *this;
}
