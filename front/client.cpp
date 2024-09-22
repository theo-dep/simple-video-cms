#include "client.h"

#include "servercommon.h"
#include "stringutils.h"

#include <httplib.h>

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

namespace client
{
    std::string get_page(const httplib::Result& res) noexcept;
    httplib::Client& client() noexcept;
}

httplib::Client& client::client() noexcept
{
    static httplib::Client client(sc::get_env("SERVER_URL", "localhost:8080"));
    return client;
}

std::string client::get_404_error() noexcept
{
    const httplib::Result res{ client().Get("/html/404.html") };
    return get_page(res);
}

std::string client::get_403_error() noexcept
{
    const httplib::Result res{ client().Get("/html/403.html") };
    return get_page(res);
}

std::string client::get_generic_error(int error, const std::string& message) noexcept
{
    return std::format(generic_error_html, "Error", error, message, CPPHTTPLIB_VERSION);
}

std::string client::get_homepage() noexcept
{
    const httplib::Result res{ client().Get("/html/homepage.html") };
    return get_page(res);
}

std::string client::get_login() noexcept
{
    const httplib::Result res{ client().Get("/html/login.html") };
    return get_page(res);
}

std::vector<std::string> client::get_most_viewed() noexcept
{
    const httplib::Result res{ client().Get("/most-viewed") };
    const std::string str_ids{ get_page(res) };
    return su::split(str_ids);
}

std::string client::video_title(const std::string& id) noexcept
{
    const httplib::Result res{ client().Get("/title/" + id) };
    return get_page(res);
}

int client::video_views(const std::string& id) noexcept
{
    const httplib::Result res{ client().Get("/views/" + id) };
    return std::stoi(get_page(res));
}

std::string client::video_uploader(const std::string& id) noexcept
{
    const httplib::Result res{ client().Get("/uploader/" + id) };
    return get_page(res);
}

bool client::is_admin(const std::string& session_id) noexcept
{
    const httplib::Result res{ client().Get("/is-admin/" + session_id) };
    return su::string_to_bool(get_page(res));
}

bool client::is_valid_username(const std::string& username) noexcept
{
    const httplib::Result res{ client().Get("/is-valid-username/" + username) };
    return su::string_to_bool(get_page(res));
}

void client::add_user(const std::string& username, const std::string& password) noexcept
{
    const httplib::MultipartFormDataItems items{
        { "username", username, "", "" },
        { "password", password, "", "" }
    };
    client().Post("/add-user/" + username, items);
}

bool client::is_valid_user(const std::string& username, const std::string& password) noexcept
{
    const httplib::MultipartFormDataItems items{
        { "username", username, "", "" },
        { "password", password, "", "" }
    };
    const httplib::Result res{ client().Post("/is-valid-user", items) };
    return su::string_to_bool(get_page(res));
}

void client::update_session(const std::string& session_id, const std::string& username) noexcept
{
    const httplib::MultipartFormDataItems items{
        { "username", username, "", "" },
        { "session_id", session_id, "", "" }
    };
    client().Post("/update-session/" + username, items);
}

std::string client::get_page(const httplib::Result& res) noexcept
{
    if (!res)
        return get_generic_error(static_cast<int>(res.error()), httplib::to_string(res.error()));

    if (res->status != httplib::StatusCode::OK_200)
        return get_generic_error(res->status, httplib::status_message(res->status));

    return res->body;
}
