#pragma once

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
}

#define MSG(message) \
    std::cout << sc::log(__FUNCTION__, __LINE__, __FILE__, message) << std::endl;

#define ERR(message) \
    std::cerr << sc::log(__FUNCTION__, __LINE__, __FILE__, message) << std::endl;
