#pragma once

#include <string>

namespace httplib
{
    class Request;
    class Response;
}

namespace sc
{
    std::string time_local();
    std::string log(const httplib::Request& req, const httplib::Response& res);

    std::string get_env(const std::string& key, const std::string& default_value);
}
