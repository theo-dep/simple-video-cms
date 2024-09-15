#include "server.h"

#include "database.h"
#include "serialization.h"
#include "servercommon.h"

#include <filesystem>
#include <format>

Server::Server() : httplib::Server()
{
    set_logger([](const httplib::Request& req, const httplib::Response& res) {
        std::cout << sc::log(req, res) << std::endl;
    });

    serve_template();

    serve_most_viewed();

    serve_video_title();
    serve_video_views();
    serve_video_uploader();

    serve_is_admin();
}

int Server::start()
{
    constexpr const char* host{ "0.0.0.0" };
    constexpr int port{ 5000 };
    MSG(std::format("Serving HTTP on {0} port {1} ...", host, port));
    return (listen(host, port) ? EXIT_SUCCESS : EXIT_FAILURE);
}

void Server::serve_template()
{
    Get("/html/:html", [](const httplib::Request& req, httplib::Response& res) {
        const std::string html{ req.path_params.at("html") };
        const std::filesystem::path html_path{ std::filesystem::current_path() / "templates" / html };
        res.set_file_content(html_path.string(), "text/html");
    });
}

void Server::serve_most_viewed()
{
    Get("/most-viewed", [](const httplib::Request& /*req*/, httplib::Response& res) {
        Database db;
        const std::vector<std::string> ids{ db.most_viewed() };
        res.set_content(sz::join(ids), "plain/text");
    });
}

void Server::serve_video_title()
{
    Get("/title/:id", [](const httplib::Request& /*req*/, httplib::Response& /*res*/) {
        // TODO
    });
}

void Server::serve_video_views()
{
    Get("/views/:id", [](const httplib::Request& /*req*/, httplib::Response& /*res*/) {
        // TODO
    });
}

void Server::serve_video_uploader()
{
    Get("/uploader/:id", [](const httplib::Request& /*req*/, httplib::Response& /*res*/) {
        // TODO
    });
}

void Server::serve_is_admin()
{
    Get("/is-admin/:session_id", [](const httplib::Request& /*req*/, httplib::Response& /*res*/) {
        // TODO
    });
}
