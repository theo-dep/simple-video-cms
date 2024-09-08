#include "client.h"

#include "serialization.h"
#include "servercommon.h"

#include <format>

constexpr const char* generic_error_html{
    R"(<html>
    <head>
        <title>{0}</title>
        <link rel="stylesheet" href="static/css/404.css" type="text/css">
        <link rel="stylesheet" href="static/css/font.css" type="text/css">
        <link rel="icon" href="static/img/favicon.png" type="image/png">
    </head>
    <body class="back">
        <div class="main">
            <br><br><br><br><br><br>
            <h1>Error {1}</h1>
            <h3>{2}</h3>
            <h3>cpp-httplib/{3}<h3>
            <a href="/" class="back">BACK TO HOME</a>
        </div>
    </body>
    </html>
 )"
};

Client::Client() : httplib::Client(sc::get_env("SERVER_URL", "localhost:8080"))
{
}

std::string Client::get_404_error()
{
    const httplib::Result res{ Get("/html/404.html") };
    return get_page(res);
}

std::string Client::get_403_error()
{
    const httplib::Result res{ Get("/html/403.html") };
    return get_page(res);
}

std::string Client::get_generic_error(int error, const std::string& message)
{
    return std::format(generic_error_html, "Error", error, message, CPPHTTPLIB_VERSION);
}

std::string Client::get_homepage()
{
    const httplib::Result res{ Get("/html/homepage.html") };
    return get_page(res);
}

std::vector<std::string> Client::get_most_viewed()
{
    const httplib::Result res{ Get("/most-viewed") };
    const std::string str_ids{ get_page(res) };
    return sz::split(str_ids);
}

std::string Client::video_title(const std::string& id)
{
    const httplib::Result res{ Get("/title/" + id) };
    return get_page(res);
}

int Client::video_views(const std::string& id)
{
    const httplib::Result res{ Get("/views/" + id) };
    return std::stoi(get_page(res));
}

std::string Client::video_uploader(const std::string& id)
{
    const httplib::Result res{ Get("/uploader/" + id) };
    return get_page(res);
}

bool Client::is_admin(const std::string& session_id)
{
    const httplib::Result res{ Get("/is-admin/" + session_id) };
    return (get_page(res) == "true");
}

std::string Client::get_page(const httplib::Result& res)
{
    if (!res)
        return get_generic_error(static_cast<int>(res.error()), httplib::to_string(res.error()));

    if (res->status != httplib::StatusCode::OK_200)
        return get_generic_error(res->status, httplib::status_message(res->status));

    return res->body;
}
