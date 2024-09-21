#include "server.h"

#include "database.h"
#include "serialization.h"
#include "servercommon.h"

#include <httplib.h>

#include <filesystem>
#include <format>

namespace server
{
    void serve_template(httplib::Server& server) noexcept;
    void serve_most_viewed(httplib::Server& server) noexcept;
    void serve_video_title(httplib::Server& server) noexcept;
    void serve_video_views(httplib::Server& server) noexcept;
    void serve_video_uploader(httplib::Server& server) noexcept;
    void serve_is_admin(httplib::Server& server) noexcept;
}

int server::start() noexcept
{
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

    constexpr const char* host{ "0.0.0.0" };
    constexpr int port{ 5000 };
    MSG(std::format("Serving HTTP on {0} port {1} ...", host, port));
    return (server.listen(host, port) ? EXIT_SUCCESS : EXIT_FAILURE);
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
        res.set_content(sz::join(ids), "plain/text");
    });
}

inline void server::serve_video_title(httplib::Server& server) noexcept
{
    server.Get("/title/:id", [](const httplib::Request& /*req*/, httplib::Response& /*res*/) {
        // TODO
    });
}

inline void server::serve_video_views(httplib::Server& server) noexcept
{
    server.Get("/views/:id", [](const httplib::Request& /*req*/, httplib::Response& /*res*/) {
        // TODO
    });
}

inline void server::serve_video_uploader(httplib::Server& server) noexcept
{
    server.Get("/uploader/:id", [](const httplib::Request& /*req*/, httplib::Response& /*res*/) {
        // TODO
    });
}

inline void server::serve_is_admin(httplib::Server& server) noexcept
{
    server.Get("/is-admin/:session_id", [](const httplib::Request& /*req*/, httplib::Response& /*res*/) {
        // TODO
    });
}
