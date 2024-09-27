#include "server.h"

#include "client.h"
#include "crypto.h"
#include "servercommon.h"
#include "session.h"
#include "stringutils.h"

#include <httplib.h>
#include <inja.hpp>

#include <filesystem>
#include <format>

namespace server
{
    void set_no_cache_headers(httplib::Response& res) noexcept;

    void set_error_handler(httplib::Server& server) noexcept;
    void set_exception_handler(httplib::Server& server) noexcept;
    void set_logger(httplib::Server& server) noexcept;

    void serve_home(httplib::Server& server, inja::Environment& env, const Session& session) noexcept;
    void serve_dashboard(httplib::Server& server, inja::Environment& env, const Session& session) noexcept;

    void serve_login(httplib::Server& server, inja::Environment& env, Session& session) noexcept;
    void serve_logout(httplib::Server& server, Session& session) noexcept;
}

int server::start() noexcept
{
    inja::Environment env;
    Session session;
    httplib::Server server;

    set_error_handler(server);
    set_exception_handler(server);
    set_logger(server);

    const std::filesystem::path static_path{ std::filesystem::current_path() / "static" };
    server.set_mount_point("/static", static_path.string());

    server.set_post_routing_handler([](const httplib::Request& /*req*/, httplib::Response& res) {
        set_no_cache_headers(res);
    });

    serve_home(server, env, session);
    serve_dashboard(server, env, session);

    serve_login(server, env, session);
    serve_logout(server, session);

    constexpr const char* host{ "0.0.0.0" };
    constexpr int port{ 8080 };
    MSG(std::format("Serving HTTP on {0} port {1} ...", host, port));
    return (server.listen(host, port) ? EXIT_SUCCESS : EXIT_FAILURE);
}

inline void server::set_no_cache_headers(httplib::Response& res) noexcept
{
    res.set_header("Last-Modified", sc::time_local());
    res.set_header("Cache-Control", "no-store, no-cache, must-revalidate, post-check=0, pre-check=0, max-age=0");
    res.set_header("Pragma", "no-cache");
    res.set_header("Expires", "-1");
}

inline void server::set_error_handler(httplib::Server& server) noexcept
{
    server.set_error_handler([](const httplib::Request& /*req*/, httplib::Response& res) {
        std::string body;

        switch (res.status) {
            case httplib::StatusCode::NotFound_404:
                body = client::get_404_error();
                break;

            case httplib::StatusCode::Forbidden_403:
                body = client::get_403_error();
                break;

            default:
                body = client::get_generic_error(res.status, httplib::status_message(res.status));
        }

        // body = env.render(body, _data);
        res.set_content(body, "text/html");
    });
}

inline void server::set_exception_handler(httplib::Server& server) noexcept
{
    server.set_exception_handler([](const httplib::Request& /*req*/, httplib::Response& res, std::exception_ptr ep) {
        constexpr int error_code{ httplib::StatusCode::InternalServerError_500 };
        std::string message;
        try {
            std::rethrow_exception(ep);
        } catch (const std::exception& e) {
            message = e.what();
        } catch (...) {
            message = "Unknown Exception";
        }

        ERR(message);

        std::string body{ client::get_generic_error(error_code, message) };
        // body = env.render(body, _data);

        res.set_content(body, "text/html");
        res.status = error_code;
    });
}

inline void server::set_logger(httplib::Server& server) noexcept
{
    server.set_logger([](const httplib::Request& req, const httplib::Response& res) {
        std::cout << sc::log(req, res) << std::endl;
    });
}

inline void server::serve_home(httplib::Server& server, inja::Environment& env, const Session& session) noexcept
{
    server.Get("/", [&](const httplib::Request& req, httplib::Response& res) {
        const std::string cookie{ req.get_header_value("Cookie") };
        const std::string session_id{ Session::extract_session_id_from_cookie(cookie) };
        bool is_logged{ false };
        if (session.is_valid_session(session_id)) {
            is_logged = true;
            if (client::is_admin(session.user_from_session(session_id))) {
                res.set_redirect("/dashboard");
                return;
            }
        }

        const std::vector<std::string> most_viewed_video_ids{ client::get_most_viewed() };
        inja::json most_viewed = inja::json::array();

        for (const std::string& id : most_viewed_video_ids) {
            const std::string title{ client::video_title(id) };
            const int views{ client::video_views(id) };
            const std::string uploader{ client::video_uploader(id) };
            most_viewed[id] = { title, views, uploader };
        }

        const inja::json data{
            { "is_logged", is_logged },
            { "most_viewed", most_viewed }
        };
        MSG(data.dump());

        const std::string body{ env.render(client::get_homepage(), data) };
        res.set_content(body, "text/html");
    });
}

inline void server::serve_dashboard(httplib::Server& server, inja::Environment& env, const Session& session) noexcept
{
    server.Get("/dashboard", [&](const httplib::Request& req, httplib::Response& res) {
        const std::string cookie{ req.get_header_value("Cookie") };
        const std::string session_id{ Session::extract_session_id_from_cookie(cookie) };
        if (!session.is_valid_session(session_id)) {
            res.set_redirect("/login");
            return;
        }

        if (!client::is_admin(session.user_from_session(session_id))) {
            const std::string body{ client::get_403_error() };
            res.set_content(body, "text/html");
            return;
        }

        const inja::json data{
            { "user_count", client::user_count() },
            { "video_count", client::video_count() },
            { "view_count", client::view_count() }
        };
        MSG(data.dump());

        const std::string body{ env.render(client::dashboard(), data) };
        res.set_content(body, "text/html");
    });
}

inline void server::serve_login(httplib::Server& server, inja::Environment& env, Session& session) noexcept
{
    static const std::function<void(httplib::Response&, bool)> set_login_content{
        [&](httplib::Response& res, bool login_error) {
            const inja::json data{ { "login_error", login_error } };
            MSG(data.dump());
            const std::string body{ env.render(client::get_login(), data) };
            res.set_content(body, "text/html");
        }
    };

    server.Get("/login", [&](const httplib::Request& req, httplib::Response& res) {
              const std::string cookie{ req.get_header_value("Cookie") };
              if (session.is_valid_session_from_cookie(cookie)) {
                  res.set_redirect("/");
                  return;
              }

              set_login_content(res, false);
          })
        .Post("/login", [&](const httplib::Request& req, httplib::Response& res) {
            const std::string password{ crypto::sha512(req.get_param_value("password")) };
            if (password.empty()) {
                set_login_content(res, true);
                return;
            }

            std::string username{ req.get_param_value("username") };
            su::trim(username);
            su::lower(username);

            const bool is_valid_user{ client::is_valid_user(username, password) };
            if (is_valid_user) {
                const std::string session_id{ session.create_session(username) };
                res.set_header("Set-Cookie", Session::insert_session_id_to_cookie(session_id));
                res.set_redirect("/");
            } else {
                set_login_content(res, true);
            }
        });
}

inline void server::serve_logout(httplib::Server& server, Session& session) noexcept
{
    server.Get("/logout", [&](const httplib::Request& req, httplib::Response& res) {
        const std::string cookie{ req.get_header_value("Cookie") };
        const std::string session_id{ Session::extract_session_id_from_cookie(cookie) };
        session.remove_session(session_id);
        res.set_redirect("/");
    });
}
