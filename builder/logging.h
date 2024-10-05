#pragma once

#include <format>
#include <iostream>
#include <source_location>
#include <string>

namespace logging
{
    std::string time_local() noexcept;

    template <class... Args>
    struct log
    {
        log(std::ostream& stream, const std::source_location& location, std::format_string<Args...> fmt, Args&&... args) noexcept;
    };

    template <>
    struct log<std::string>
    {
        log(std::ostream& stream, const std::source_location& location, const std::string& message) noexcept;
    };

    // https://www.cppstories.com/2020/09/variadic-pack-first.html/
    // https://cor3ntin.github.io/posts/variadic/
    // https://en.cppreference.com/w/cpp/language/class_template_argument_deduction#User-defined_deduction_guides
    template <class... Args>
    struct info
    {
        info(std::format_string<Args...> fmt, Args&&... args, const std::source_location& location = std::source_location::current()) noexcept;
    };
    template <class... Args>
    info(std::format_string<Args...> fmt, Args&&... args) -> info<Args...>;

    template <>
    struct info<std::string>
    {
        info(const std::string& message, const std::source_location& location = std::source_location::current()) noexcept;
    };
    info(const std::string& message) -> info<std::string>;

    template <class... Args>
    struct error
    {
        error(std::format_string<Args...> fmt, Args&&... args, const std::source_location& location = std::source_location::current()) noexcept;
    };
    template <class... Args>
    error(std::format_string<Args...> fmt, Args&&... args) -> error<Args...>;

    template <>
    struct error<std::string>
    {
        error(const std::string& message, const std::source_location& location = std::source_location::current()) noexcept;
    };
    error(const std::string& message) -> error<std::string>;

    template <class... Args>
    struct debug
    {
        debug(std::format_string<Args...> fmt, Args&&... args, const std::source_location& location = std::source_location::current()) noexcept;
    };
    template <class... Args>
    debug(std::format_string<Args...> fmt, Args&&... args) -> debug<Args...>;

    template <>
    struct debug<std::string>
    {
        debug(const std::string& message, const std::source_location& location = std::source_location::current()) noexcept;
    };
    debug(const std::string& message) -> debug<std::string>;
}

template <class... Args>
logging::log<Args...>::log(std::ostream& stream, const std::source_location& location, std::format_string<Args...> fmt, Args&&... args) noexcept
{
    try {
        log<std::string>{ stream, location, std::format(fmt, std::forward<Args>(args)...) };
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}

template <class... Args>
logging::info<Args...>::info(std::format_string<Args...> fmt, Args&&... args, const std::source_location& location) noexcept
{
    log<Args...>{ std::cout, location, fmt, std::forward<Args>(args)... };
}

template <class... Args>
logging::error<Args...>::error(std::format_string<Args...> fmt, Args&&... args, const std::source_location& location) noexcept
{
    log<Args...>{ std::cerr, location, fmt, std::forward<Args>(args)... };
}

template <class... Args>
logging::debug<Args...>::debug(std::format_string<Args...> fmt, Args&&... args, const std::source_location& location) noexcept
{
    // reuse the macro formatting
    try {
        debug<std::string>{ std::format(fmt, std::forward<Args>(args)...), location };
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}
