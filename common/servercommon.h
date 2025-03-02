#pragma once

#include <functional>
#include <string>

namespace httplib
{
    struct Request;
    struct Response;
}

namespace sc
{
    std::string log(const httplib::Request& req, const httplib::Response& res);

    std::string get_env(const std::string& key, const std::string& default_value);

    template <typename... Args>
    std::function<void(const httplib::Request&, httplib::Response&)> serve(
        const std::function<void(const httplib::Request&, httplib::Response&, std::decay_t<Args>...)>& handler,
        Args&&... args);

    constexpr std::string static_regexp_path();
}

template <typename... Args>
inline std::function<void(const httplib::Request&, httplib::Response&)> sc::serve(
    const std::function<void(const httplib::Request&, httplib::Response&, std::decay_t<Args>...)>& handler,
    Args&&... args)
{
    return std::bind(handler, std::placeholders::_1, std::placeholders::_2, std::forward<Args>(args)...);
}

constexpr std::string sc::static_regexp_path()
{
    using namespace std::literals;
    // /static/((stem).(extension))
    // 0: all
    // 1: filename (with path)
    // 2: stem (with path)
    // 3: extension
    return R"(\/static\/(([\w\/\-]+)\.(\w)+))"s;
}
