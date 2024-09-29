#include "client.h"

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

    std::string format_page(const httplib::Result& res) noexcept;
    httplib::Client& client();
}

// construction exception catched by httplib Server::set_exception_handler
httplib::Client& client::client()
{
    static httplib::Client client(sc::get_env("SERVER_URL", "localhost:8080"));
    return client;
}

std::string client::error_page_404() noexcept
{
    const httplib::Result res{ client().Get("/html/404.html") };
    return format_page(res);
}

std::string client::error_page_403() noexcept
{
    const httplib::Result res{ client().Get("/html/403.html") };
    return format_page(res);
}

std::string client::generic_error(int error, const std::string& message) noexcept
{
    try {
        return std::format(generic_error_html, "Error", error, message, CPPHTTPLIB_VERSION);
    } catch (const std::exception& e) {
        return e.what();
    }
}

std::string client::home_page() noexcept
{
    const httplib::Result res{ client().Get("/html/homepage.html") };
    return format_page(res);
}

std::string client::dashboard_page() noexcept
{
    const httplib::Result res{ client().Get("/html/dashboard.html") };
    return format_page(res);
}

std::string client::login_page() noexcept
{
    const httplib::Result res{ client().Get("/html/login.html") };
    return format_page(res);
}

std::string client::confirm_action_page() noexcept
{
    const httplib::Result res{ client().Get("/html/confirm_action.html") };
    return format_page(res);
}

std::string client::user_list_page() noexcept
{
    const httplib::Result res{ client().Get("/html/user_list.html") };
    return format_page(res);
}

std::string client::add_user_page() noexcept
{
    const httplib::Result res{ client().Get("/html/add_user.html") };
    return format_page(res);
}

std::string client::update_user_page() noexcept
{
    const httplib::Result res{ client().Get("/html/update_user.html") };
    return format_page(res);
}

std::vector<std::string> client::most_viewed_video_list() noexcept
{
    const httplib::Result res{ client().Get("/most-viewed") };
    const std::string str_ids{ format_page(res) };
    return su::split(str_ids);
}

std::string client::video_title(const std::string& id) noexcept
{
    const httplib::Result res{ client().Get("/title/" + id) };
    return format_page(res);
}

int client::video_views(const std::string& id) noexcept
{
    const httplib::Result res{ client().Get("/views/" + id) };
    return su::string_to_int(format_page(res));
}

std::string client::video_uploader(const std::string& id) noexcept
{
    const httplib::Result res{ client().Get("/uploader/" + id) };
    return format_page(res);
}

bool client::is_admin(const std::string& username) noexcept
{
    const httplib::Headers headers{
        { "username", username },
    };
    const httplib::Result res{ client().Get("/is-admin", headers) };
    return su::string_to_bool(format_page(res));
}

bool client::is_valid_username(const std::string& username) noexcept
{
    const httplib::Headers headers{
        { "username", username },
    };
    const httplib::Result res{ client().Get("/is-valid-username", headers) };
    return su::string_to_bool(format_page(res));
}

void client::add_user(const std::string& username, const std::string& password) noexcept
{
    const httplib::MultipartFormDataItems items{
        { "username", username, "", "" },
        { "password", password, "", "" }
    };
    client().Post("/add-user", items);
}

void client::update_user(const std::string& username, const std::string& password) noexcept
{
    const httplib::MultipartFormDataItems items{
        { "username", username, "", "" },
        { "password", password, "", "" }
    };
    client().Post("/update-user", items);
}

void client::delete_user(const std::string& username) noexcept
{
    const httplib::MultipartFormDataItems items{
        { "username", username, "", "" },
    };
    client().Post("/delete-user", items);
}

bool client::is_valid_user(const std::string& username, const std::string& password) noexcept
{
    const httplib::MultipartFormDataItems items{
        { "username", username, "", "" },
        { "password", password, "", "" }
    };
    const httplib::Result res{ client().Post("/is-valid-user", items) };
    return su::string_to_bool(format_page(res));
}

int client::user_count() noexcept
{
    const httplib::Result res{ client().Get("/user-count") };
    return su::string_to_int(format_page(res));
}

int client::video_count() noexcept
{
    const httplib::Result res{ client().Get("/video-count") };
    return su::string_to_int(format_page(res));
}

int client::view_count() noexcept
{
    const httplib::Result res{ client().Get("/view-count") };
    return su::string_to_int(format_page(res));
}

std::vector<std::string> client::user_list() noexcept
{
    const httplib::Result res{ client().Get("/user-list") };
    const std::string str_usernames{ format_page(res) };
    return su::split(str_usernames);
}

std::string client::format_page(const httplib::Result& res) noexcept
{
    if (!res)
        return generic_error(static_cast<int>(res.error()), httplib::to_string(res.error()));

    if (res->status != httplib::StatusCode::OK_200)
        return generic_error(res->status, httplib::status_message(res->status));

    return res->body;
}
