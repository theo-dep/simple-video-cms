#include "servercommon.h"

#include "logging.h"

#include <httplib.h>

std::string sc::log(const httplib::Request& req, const httplib::Response& res)
{
    const std::string remote_addr{ req.get_header_value("X-Forwarded-For", req.remote_addr.c_str()) };
    const std::string remote_user{ "-" }; // TODO:
    const std::string request{ std::format("{} {} {}", req.method, req.path, req.version) };
    const std::string body_bytes_sent{ res.get_header_value("Content-Length") };
    const std::string http_referer{ "-" }; // TODO:
    const std::string http_user_agent{ req.get_header_value("User-Agent", "-") };

    // NOTE: From NGINX default access log format
    // log_format combined '$remote_addr - $remote_user [$time_local] '
    //                     '"$request" $status $body_bytes_sent '
    //                     '"$http_referer" "$http_user_agent"';
    return std::format(R"({} - {} [{}] "{}" {} {} "{}" "{}")",
                       remote_addr, remote_user, logging::time_local(), request,
                       res.status, body_bytes_sent, http_referer, http_user_agent);
}

std::string sc::get_env(const std::string& key, const std::string& default_value)
{
    const char* const db_url_env{ std::getenv(key.c_str()) };
    return (db_url_env != nullptr ? std::string(db_url_env) : default_value);
}
