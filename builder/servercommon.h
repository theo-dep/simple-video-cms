#pragma once

#include <functional>
#include <memory>
#include <string>

namespace httplib
{
    struct Request;
    struct Response;
}

namespace sc
{
    std::string log(const httplib::Request& req, const httplib::Response& res) noexcept;

    std::string get_env(const std::string& key, const std::string& default_value) noexcept;

    template <typename... Args>
    std::function<void(const httplib::Request&, httplib::Response&)> serve(
        const std::function<void(const httplib::Request&, httplib::Response&, std::decay_t<Args>...)>& handler,
        Args&&... args) noexcept;

    constexpr std::string static_regexp_path() noexcept;

    template <class T, class Deleter = std::default_delete<T>>
    struct ContentProviderReleaser
    {
        T* ptr;
        Deleter deleter;

        ContentProviderReleaser(std::unique_ptr<T, Deleter>&& u) noexcept;
        void operator()(bool) const noexcept;
    };
}

template <typename... Args>
inline std::function<void(const httplib::Request&, httplib::Response&)> sc::serve(
    const std::function<void(const httplib::Request&, httplib::Response&, std::decay_t<Args>...)>& handler,
    Args&&... args) noexcept
{
    return std::bind(handler, std::placeholders::_1, std::placeholders::_2, std::forward<Args>(args)...);
}

constexpr std::string sc::static_regexp_path() noexcept
{
    using namespace std::literals;
    // /static/((stem).(extension))
    // 0: all
    // 1: filename (with path)
    // 2: stem (with path)
    // 3: extension
    return R"(\/static\/(([\w\/\-]+)\.(\w)+))"s;
}

template <class T, class Deleter>
inline sc::ContentProviderReleaser<T, Deleter>::ContentProviderReleaser(std::unique_ptr<T, Deleter>&& u) noexcept
    : ptr{ u.release() }
    , deleter{ u.get_deleter() }
{
}

template <class T, class Deleter>
inline void sc::ContentProviderReleaser<T, Deleter>::operator()(bool) const noexcept
{
    deleter(ptr);
}
