#include "client.h"

#include "logging.h"
#include "servercommon.h"
#include "stringutils.h"

#include <httplib.h>

#include <format>

namespace client
{
    constexpr const char* generic_error_html{
        R"(<html>
           <head>
               <title>{0}</title>
               <link rel="stylesheet" href="/static/css/404.css" type="text/css">
               <link rel="stylesheet" href="/static/css/font.css" type="text/css">
               <link rel="icon" href="/static/img/favicon.png" type="image/png">
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

    std::string generic_error(int error, const std::string& message) noexcept;
    std::string format_page(const httplib::Result& res) noexcept;
}

Client::Client(bool& create_ok) noexcept
try : _client{ std::make_unique<httplib::Client>(sc::get_env("SERVER_URL", "localhost:8080")) } {
    create_ok = true;
} catch (const std::exception& e) {
    logging::error{ "Fail to create: {}", e.what() };
    create_ok = false;
}

std::string Client::error_page_404() const noexcept
{
    const httplib::Result res{ _client->Get("/html/404.html") };
    return client::format_page(res);
}

std::string Client::error_page_403() const noexcept
{
    const httplib::Result res{ _client->Get("/html/403.html") };
    return client::format_page(res);
}

std::string Client::generic_error(int error, const std::string& message) noexcept
{
    return client::generic_error(error, message);
}

std::string Client::home_page() const noexcept
{
    const httplib::Result res{ _client->Get("/html/homepage.html") };
    return client::format_page(res);
}

std::string Client::dashboard_page() const noexcept
{
    const httplib::Result res{ _client->Get("/html/dashboard.html") };
    return client::format_page(res);
}

std::string Client::login_page() const noexcept
{
    const httplib::Result res{ _client->Get("/html/login.html") };
    return client::format_page(res);
}

std::string Client::confirm_action_page() const noexcept
{
    const httplib::Result res{ _client->Get("/html/confirm_action.html") };
    return client::format_page(res);
}

std::string Client::user_list_page() const noexcept
{
    const httplib::Result res{ _client->Get("/html/user_list.html") };
    return client::format_page(res);
}

std::string Client::add_user_page() const noexcept
{
    const httplib::Result res{ _client->Get("/html/add_user.html") };
    return client::format_page(res);
}

std::string Client::update_user_page() const noexcept
{
    const httplib::Result res{ _client->Get("/html/update_user.html") };
    return client::format_page(res);
}

std::string Client::video_list_page() const noexcept
{
    const httplib::Result res{ _client->Get("/html/video_list.html") };
    return client::format_page(res);
}

std::string Client::add_video_page() const noexcept
{
    const httplib::Result res{ _client->Get("/html/add_video.html") };
    return client::format_page(res);
}

std::vector<std::string> Client::video_list() const noexcept
{
    const httplib::Result res{ _client->Get("/video-list") };
    const std::string str_ids{ client::format_page(res) };
    return su::split(str_ids);
}

std::vector<std::string> Client::most_viewed_video_list() const noexcept
{
    const httplib::Result res{ _client->Get("/most-viewed") };
    const std::string str_ids{ client::format_page(res) };
    return su::split(str_ids);
}

std::string Client::video_title(const std::string& id) const noexcept
{
    const httplib::Result res{ _client->Get("/title/" + id) };
    return client::format_page(res);
}

int Client::video_views(const std::string& id) const noexcept
{
    const httplib::Result res{ _client->Get("/views/" + id) };
    return su::string_to_int(client::format_page(res));
}

bool Client::is_admin(const std::string& username) const noexcept
{
    const httplib::Headers headers{
        { "username", username },
    };
    const httplib::Result res{ _client->Get("/is-admin", headers) };
    return su::string_to_bool(client::format_page(res));
}

bool Client::is_valid_username(const std::string& username) const noexcept
{
    const httplib::Headers headers{
        { "username", username },
    };
    const httplib::Result res{ _client->Get("/is-valid-username", headers) };
    return su::string_to_bool(client::format_page(res));
}

void Client::add_user(const std::string& username, const std::string& password) const noexcept
{
    const httplib::MultipartFormDataItems items{
        { "username", username, "", "" },
        { "password", password, "", "" }
    };
    _client->Post("/add-user", items);
}

void Client::update_user(const std::string& username, const std::string& password) const noexcept
{
    const httplib::MultipartFormDataItems items{
        { "username", username, "", "" },
        { "password", password, "", "" }
    };
    _client->Post("/update-user", items);
}

void Client::delete_user(const std::string& username) const noexcept
{
    const httplib::MultipartFormDataItems items{
        { "username", username, "", "" },
    };
    _client->Post("/delete-user", items);
}

bool Client::is_valid_user(const std::string& username, const std::string& password) const noexcept
{
    const httplib::MultipartFormDataItems items{
        { "username", username, "", "" },
        { "password", password, "", "" }
    };
    const httplib::Result res{ _client->Post("/is-valid-user", items) };
    return su::string_to_bool(client::format_page(res));
}

int Client::user_count() const noexcept
{
    const httplib::Result res{ _client->Get("/user-count") };
    return su::string_to_int(client::format_page(res));
}

int Client::video_count() const noexcept
{
    const httplib::Result res{ _client->Get("/video-count") };
    return su::string_to_int(client::format_page(res));
}

int Client::view_count() const noexcept
{
    const httplib::Result res{ _client->Get("/view-count") };
    return su::string_to_int(client::format_page(res));
}

std::vector<std::string> Client::user_list() const noexcept
{
    const httplib::Result res{ _client->Get("/user-list") };
    const std::string str_usernames{ client::format_page(res) };
    return su::split(str_usernames);
}

void Client::add_video(const std::string& title, const std::string& content) const noexcept
{
    const httplib::MultipartFormDataItems items{
        { "title", title, "", "" },
        { "video", content, "", "" }
    };
    _client->Post("/add-video", items);
}

void Client::delete_video(const std::string& id) const noexcept
{
    _client->Post("/delete-video/" + id);
}

inline std::string client::generic_error(int error, const std::string& message) noexcept
{
    try {
        return std::format(generic_error_html, "Error", error, message, CPPHTTPLIB_VERSION);
    } catch (const std::exception& e) {
        return e.what();
    }
}

inline std::string client::format_page(const httplib::Result& res) noexcept
{
    if (!res)
        return generic_error(static_cast<int>(res.error()), httplib::to_string(res.error()));

    if (res->status != httplib::StatusCode::OK_200)
        return generic_error(res->status, httplib::status_message(res->status));

    return res->body;
}
