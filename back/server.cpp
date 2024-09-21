#include "server.h"

#include "database.h"
#include "servercommon.h"
#include "stringutils.h"

#include <httplib.h>

#include <filesystem>
#include <format>

namespace server
{
    void create_admin() noexcept;

    void serve_template(httplib::Server& server) noexcept;
    void serve_most_viewed(httplib::Server& server) noexcept;
    void serve_video_title(httplib::Server& server) noexcept;
    void serve_video_views(httplib::Server& server) noexcept;
    void serve_video_uploader(httplib::Server& server) noexcept;
    void serve_is_admin(httplib::Server& server) noexcept;
    void serve_is_valid_username(httplib::Server& server) noexcept;
    void serve_add_user(httplib::Server& server) noexcept;
    void serve_is_valid_user(httplib::Server& server) noexcept;
    void serve_update_session(httplib::Server& server) noexcept;
}

int server::start() noexcept
{
    create_admin();

    httplib::Server server;
    server.set_logger([](const httplib::Request& req, const httplib::Response& res) {
        std::cout << sc::log(req, res) << std::endl;
    });

    serve_template(server);

    serve_most_viewed(server);

    serve_video_title(server);
    serve_video_views(server);
    serve_video_uploader(server);

    serve_is_admin(server);
    serve_is_valid_username(server);
    serve_add_user(server);
    serve_is_valid_user(server);
    serve_update_session(server);

    constexpr const char* host{ "0.0.0.0" };
    constexpr int port{ 5000 };
    MSG(std::format("Serving HTTP on {0} port {1} ...", host, port));
    return (server.listen(host, port) ? EXIT_SUCCESS : EXIT_FAILURE);
}

inline void server::create_admin() noexcept
{
    const std::string username{ sc::get_env("MYSQL_ADMIN_USERNAME", "admin") };
    const std::string password{ su::sha512(sc::get_env("MYSQL_ADMIN_PASSWORD", "admin")) };
    database::add_admin(username, password);
}

inline void server::serve_template(httplib::Server& server) noexcept
{
    server.Get("/html/:html", [](const httplib::Request& req, httplib::Response& res) {
        const std::string html{ req.path_params.at("html") };
        const std::filesystem::path html_path{ std::filesystem::current_path() / "templates" / html };
        res.set_file_content(html_path.string(), "text/html");
    });
}

inline void server::serve_most_viewed(httplib::Server& server) noexcept
{
    server.Get("/most-viewed", [](const httplib::Request& /*req*/, httplib::Response& res) {
        const std::vector<std::string> ids{ database::most_viewed() };
        res.set_content(su::join(ids), "plain/text");
    });
}

inline void server::serve_video_title(httplib::Server& server) noexcept
{
    server.Get("/title/:video_id", [](const httplib::Request& req, httplib::Response& res) {
        const std::string video_id{ req.path_params.at("video_id") };
        const std::string video_title{ database::video_title(video_id) };
        res.set_content(video_title, "plain/text");
    });
}

inline void server::serve_video_views(httplib::Server& server) noexcept
{
    server.Get("/views/:video_id", [](const httplib::Request& req, httplib::Response& res) {
        const std::string video_id{ req.path_params.at("video_id") };
        const int video_views{ database::video_views(video_id) };
        res.set_content(std::to_string(video_views), "plain/text");
    });
}

inline void server::serve_video_uploader(httplib::Server& server) noexcept
{
    server.Get("/uploader/:video_id", [](const httplib::Request& req, httplib::Response& res) {
        const std::string video_id{ req.path_params.at("video_id") };
        const std::string video_uploader{ database::video_uploader(video_id) };
        res.set_content(video_uploader, "plain/text");
    });
}

inline void server::serve_is_admin(httplib::Server& server) noexcept
{
    server.Get("/is-admin/:session_id", [](const httplib::Request& req, httplib::Response& res) {
        const std::string session_id{ req.path_params.at("session_id") };
        const std::optional<bool> is_admin{ database::is_admin(session_id) };
        if (!is_admin.has_value()) {
            res.set_content("", "plain/text");
            return;
        }

        res.set_content(is_admin.value() ? "true" : "false", "plain/text");
    });
}

inline void server::serve_is_valid_username(httplib::Server& server) noexcept
{
    server.Get("/is-valid-username/:username", [](const httplib::Request& req, httplib::Response& res) {
        const std::string username{ req.path_params.at("username") };
        const bool is_valid_username{ database::is_valid_username(username) };
        res.set_content(su::bool_to_string(is_valid_username), "plain/text");
    });
}

inline void server::serve_add_user(httplib::Server& server) noexcept
{
    server.Post("/add-user/:username", [](const httplib::Request& req, httplib::Response& res) {
        const std::string username{ req.path_params.at("username") };
        if (!req.has_file("password") || req.get_file_value("username").content != username) {
            ERR("Missing multipart form data");
            res.status = httplib::StatusCode::InternalServerError_500;
            return;
        }

        const std::string password{ req.get_file_value("password").content };
        database::add_user(username, password);
    });
}

inline void server::serve_is_valid_user(httplib::Server& server) noexcept
{
    server.Post("/is-valid-user", [](const httplib::Request& req, httplib::Response& res) {
        if (!req.has_file("password") || !req.has_file("username")) {
            ERR("Missing multipart form data");
            res.status = httplib::StatusCode::InternalServerError_500;
            return;
        }

        const std::string username{ req.get_file_value("username").content };
        const std::string password{ req.get_file_value("password").content };

        const std::string database_password{ database::get_password(username) };
        res.set_content(su::bool_to_string(password == database_password), "plain/text");
    });
}

inline void server::serve_update_session(httplib::Server& server) noexcept
{
    server.Post("/update-session/:username", [](const httplib::Request& req, httplib::Response& res) {
        const std::string username{ req.path_params.at("username") };
        if (!req.has_file("session_id") || req.get_file_value("username").content != username) {
            ERR("Missing multipart form data");
            res.status = httplib::StatusCode::InternalServerError_500;
            return;
        }

        const std::string session_id{ req.get_file_value("session_id").content };
        database::update_session(session_id, username);
    });
}
