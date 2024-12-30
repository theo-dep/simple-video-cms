#pragma once

#include <filesystem>
#include <format>
#include <source_location>
#include <string>

namespace logging
{
    void init(const std::filesystem::path& log_file_path);

    std::string time_local();
    void raw_log(const std::string& message); // for server log

    // https://www.cppstories.com/2020/09/variadic-pack-first.html/
    // https://cor3ntin.github.io/posts/variadic/
    // https://en.cppreference.com/w/cpp/language/class_template_argument_deduction#User-defined_deduction_guides
    template <class... Args>
    struct info
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
    struct error
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
    struct debug
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
