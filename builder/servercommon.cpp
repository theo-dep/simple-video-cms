#include "servercommon.h"

#include <httplib.h>

#include <chrono>
#include <format>

std::string sc::time_local()
{
    const std::chrono::time_point p{ std::chrono::system_clock::now() };
    const std::time_t t{ std::chrono::system_clock::to_time_t(p) };

    std::stringstream ss;
    ss << std::put_time(std::localtime(&t), "%d/%b/%Y:%H:%M:%S %z");
    return ss.str();
}

std::string sc::log(const httplib::Request& req, const httplib::Response& res)
{
    const std::string remote_user("-"); // TODO:
    const std::string request(std::format("{} {} {}", req.method, req.path, req.version));
    const std::string body_bytes_sent(res.get_header_value("Content-Length"));
    const std::string http_referer("-"); // TODO:
    const std::string http_user_agent(req.get_header_value("User-Agent", "-"));

    // NOTE: From NGINX default access log format
    // log_format combined '$remote_addr - $remote_user [$time_local] '
    //                     '"$request" $status $body_bytes_sent '
    //                     '"$http_referer" "$http_user_agent"';
    return std::format(R"({} - {} [{}] "{}" {} {} "{}" "{}")", req.remote_addr,
                       remote_user, time_local(), request, res.status,
                       body_bytes_sent, http_referer, http_user_agent);
}

std::string sc::log(const char* func, int line, const char* file, const std::string& message)
{
    return std::format(R"({} - {} - {} ({} at line {}))", time_local(), message, func, file, line);
}

std::string sc::get_env(const std::string& key, const std::string& default_value)
{
    const char* const db_url_env{ std::getenv(key.c_str()) };
    return (db_url_env ? std::string(db_url_env) : default_value);
}
