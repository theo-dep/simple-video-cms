#pragma once

#include <cstdint>
#include <format>
#include <functional>
#include <iostream>
#include <string>

namespace httplib
{
    class Request;
    class Response;
}

namespace sc
{
    std::string time_local() noexcept;
    std::string log(const httplib::Request& req, const httplib::Response& res) noexcept;
    std::string log(const char* func, int line, const char* file, const std::string& message) noexcept;
    template <class... Args>
    std::string log(const char* func, int line, const char* file, std::format_string<Args...> fmt, Args&&... args) noexcept;

    std::string get_env(const std::string& key, const std::string& default_value) noexcept;

    enum class ERequestMethod : std::uint8_t
    {
        GET,
        POST
    };

    template <typename Handler, typename... Args>
    std::function<void(const httplib::Request&, httplib::Response&)> serve(Handler handler, Args... args) noexcept;
}

#define MSG(message, ...) \
    std::cout << sc::log(__FUNCTION__, __LINE__, __FILE__, message __VA_OPT__(, ) __VA_ARGS__) << '\n';

#define ERR(message, ...) \
    std::cerr << sc::log(__FUNCTION__, __LINE__, __FILE__, message __VA_OPT__(, ) __VA_ARGS__) << '\n';

#ifdef DEBUG_LOG
#define DEBUG(message, ...) MSG(message __VA_OPT__(, ) __VA_ARGS__)
#else
#define DEBUG(message, ...)
#endif

template <class... Args>
std::string sc::log(const char* func, int line, const char* file, std::format_string<Args...> fmt, Args&&... args) noexcept
{
    try {
        return log(func, line, file, std::format(fmt, std::forward<Args>(args)...));
    } catch (const std::exception& e) {
        return e.what();
    }
}

template <typename Handler, typename... Args>
std::function<void(const httplib::Request&, httplib::Response&)> sc::serve(Handler handler, Args... args) noexcept
{
    return std::bind(handler, std::placeholders::_1, std::placeholders::_2, std::forward<Args>(args)...);
}
