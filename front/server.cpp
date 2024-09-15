#include "server.h"

#include "client.h"
#include "serialization.h"
#include "servercommon.h"

#include <algorithm>
#include <format>
#include <iostream>
#include <ranges>

Server::Server() : httplib::Server()
{
    _env.add_callback("url_for", [](const inja::Arguments& args) {
        constexpr char delim = '/';
        std::vector<std::string> str_args(args.size());
        std::ranges::transform(args, str_args.begin(), [](const inja::json* val) -> std::string { return val->get<std::string>(); });
        const std::string url{ sz::join(str_args, delim) };
        return url;
    });

    set_error_handler();
    set_exception_handler();
    set_logger();

    set_mount_point("/static", "./static");

    set_post_routing_handler([this](const httplib::Request& /*req*/, httplib::Response& res) {
        set_no_cache_headers(res);
    });

    serve_home();
}

int Server::start()
{
    constexpr const char* host{ "0.0.0.0" };
    constexpr int port{ 8080 };
    std::cout << std::format("Serving HTTP on {0} port {1} ...", host, port) << std::endl;
    return (listen(host, port) ? EXIT_SUCCESS : EXIT_FAILURE);
}

void Server::set_no_cache_headers(httplib::Response& res)
{
    res.set_header("Last-Modified", sc::time_local());
    res.set_header("Cache-Control", "no-store, no-cache, must-revalidate, post-check=0, pre-check=0, max-age=0");
    res.set_header("Pragma", "no-cache");
    res.set_header("Expires", "-1");
}

void Server::set_error_handler()
{
    httplib::Server::set_error_handler([this](const httplib::Request& /*req*/, httplib::Response& res) {
        std::string body;

        Client cli;
        switch (res.status) {
            case httplib::StatusCode::NotFound_404:
                body = cli.get_404_error();
                break;

            case httplib::StatusCode::Forbidden_403:
                body = cli.get_403_error();
                break;

            default:
                body = cli.get_generic_error(res.status, httplib::status_message(res.status));
        }

        body = _env.render(body, _data);
        res.set_content(body, "text/html");
    });
}

void Server::set_exception_handler()
{
    httplib::Server::set_exception_handler([this](const httplib::Request& /*req*/, httplib::Response& res, std::exception_ptr ep) {
        constexpr int error_code{ httplib::StatusCode::InternalServerError_500 };
        std::string message;
        try {
            std::rethrow_exception(ep);
        } catch (const std::exception& e) {
            message = e.what();
        } catch (...) {
            message = "Unknown Exception";
        }

        Client cli;
        std::string body{ cli.get_generic_error(error_code, message) };
        // body = _env.render(body, _data);

        res.set_content(body, "text/html");
        res.status = error_code;
    });
}

void Server::set_logger()
{
    httplib::Server::set_logger([](const httplib::Request& req, const httplib::Response& res) {
        std::cout << sc::log(req, res) << std::endl;
    });
}

void Server::serve_home()
{
    Get("/", [this](const httplib::Request& req, httplib::Response& res) {
        Client cli;

        const std::string cookie{ req.get_header_value("Cookie") };
        const std::string session_id{ Session::extract_session_id_from_cookie(cookie) };
        bool is_logged{ false };
        if (_session.is_valid_session(session_id)) {
            is_logged = true;
            const bool is_admin{ cli.is_admin(session_id) };
            if (is_admin) {
                res.set_redirect("/dashboard");
                return;
            }
        }

        const std::vector<std::string> most_viewed_video_ids{ cli.get_most_viewed() };
        inja::json most_viewed{ inja::json::array() };

        for (const std::string& id : most_viewed_video_ids) {
            const std::string title{ cli.video_title(id) };
            const int views{ cli.video_views(id) };
            const std::string uploader{ cli.video_uploader(id) };
            most_viewed[id] = { title, views, uploader };
        }

        _data["is_logged"] = is_logged;
        _data["most_viewed"] = most_viewed;

        const std::string body{ _env.render(cli.get_homepage(), _data) };
        res.set_content(body, "text/html");
    });
}
