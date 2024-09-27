#pragma once

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

    std::string get_env(const std::string& key, const std::string& default_value) noexcept;

    enum class ERequestMethod
    {
        GET,
        POST
    };

    template <typename Handler, typename... Args>
    std::function<void(const httplib::Request&, httplib::Response&)> serve(Handler handler, Args... args) noexcept;
}

#define MSG(message) \
    std::cout << sc::log(__FUNCTION__, __LINE__, __FILE__, message) << std::endl;

#define ERR(message) \
    std::cerr << sc::log(__FUNCTION__, __LINE__, __FILE__, message) << std::endl;

#ifdef DEBUG_LOG
#define DEBUG(message) MSG(message)
#else
#define DEBUG(message)
#endif

template <typename Handler, typename... Args>
std::function<void(const httplib::Request&, httplib::Response&)> sc::serve(Handler handler, Args... args) noexcept
{
    return std::bind(handler, std::placeholders::_1, std::placeholders::_2, std::forward<Args>(args)...);
}
