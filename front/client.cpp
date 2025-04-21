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
               <title>{0} - {{{{ website_name }}}}</title>
               <meta name="viewport" content="width=device-width, initial-scale=1.0">
               <link rel="stylesheet" href="/static/css/third-party/pure-min.css" type="text/css">
               <link rel="stylesheet" href="/static/css/styles.css" type="text/css">
               <link rel="stylesheet" href="/static/css/404.css" type="text/css">
               <link rel="icon" href="{{{{ icon }}}}" type="image/png">
           </head>
           <body>
               <div class="content">
                   <div class="info">
                       <h1>Error {1}</h1>
                       <h3>{2}</h3>
                       <h3>cpp-httplib/{3}<h3>
                       <a href="/" class="back">BACK TO HOME</a>
                    </div>
               </div>
           </body>
           </html>
        )"
    };

    std::string generic_error(int error, const std::string& message);
    std::string format_page(const httplib::Result& res);
}

Client::Client()
    : _client{ std::make_unique<httplib::Client>(sc::get_env("BACK_SERVER_URL", "localhost:5000")) }
{
}

std::string Client::error_page_404() const
{
    const httplib::Result res{ _client->Get("/html/404.html") };
    return client::format_page(res);
}

std::string Client::error_page_403() const
{
    const httplib::Result res{ _client->Get("/html/403.html") };
    return client::format_page(res);
}

std::string Client::generic_error(int error, const std::string& message)
{
    return client::generic_error(error, message);
}

std::pair<std::string, std::string> Client::static_file(const std::string& file) const
{
    const httplib::Result res{ _client->Get("/static/" + file) };

    if (!res) {
        logging::error{ R"(Fail to get static file "{}" with error: {} ({}))", file, httplib::to_string(res.error()), static_cast<int>(res.error()) };
        return {};
    }

    if (res->status != httplib::StatusCode::OK_200) {
        logging::error{ R"(Fail to get static file "{}" with error: {} ({}))", file, httplib::status_message(res->status), res->status };
        return {};
    }

    return std::make_pair(res->body, res->get_header_value("Content-Type"));
}

std::string Client::home_page() const
{
    const httplib::Result res{ _client->Get("/html/homepage.html") };
    return client::format_page(res);
}

std::string Client::dashboard_page() const
{
    const httplib::Result res{ _client->Get("/html/dashboard.html") };
    return client::format_page(res);
}

std::string Client::login_page() const
{
    const httplib::Result res{ _client->Get("/html/login.html") };
    return client::format_page(res);
}

std::string Client::confirm_action_page() const
{
    const httplib::Result res{ _client->Get("/html/confirm_action.html") };
    return client::format_page(res);
}

std::string Client::user_list_page() const
{
    const httplib::Result res{ _client->Get("/html/user_list.html") };
    return client::format_page(res);
}

std::string Client::add_user_page() const
{
    const httplib::Result res{ _client->Get("/html/add_user.html") };
    return client::format_page(res);
}

std::string Client::add_password_page() const
{
    const httplib::Result res{ _client->Get("/html/add_password.html") };
    return client::format_page(res);
}

std::string Client::update_user_admin_page() const
{
    const httplib::Result res{ _client->Get("/html/update_user_admin.html") };
    return client::format_page(res);
}

std::string Client::update_user_self_page() const
{
    const httplib::Result res{ _client->Get("/html/update_user_self.html") };
    return client::format_page(res);
}

std::string Client::admin_list_page() const
{
    const httplib::Result res{ _client->Get("/html/admin_list.html") };
    return client::format_page(res);
}

std::string Client::group_list_page() const
{
    const httplib::Result res{ _client->Get("/html/group_list.html") };
    return client::format_page(res);
}

std::string Client::add_group_page() const
{
    const httplib::Result res{ _client->Get("/html/add_group.html") };
    return client::format_page(res);
}

std::string Client::update_group_page() const
{
    const httplib::Result res{ _client->Get("/html/update_group.html") };
    return client::format_page(res);
}

std::string Client::video_list_page() const
{
    const httplib::Result res{ _client->Get("/html/video_list.html") };
    return client::format_page(res);
}

std::string Client::add_video_page() const
{
    const httplib::Result res{ _client->Get("/html/add_video.html") };
    return client::format_page(res);
}

std::string Client::update_video_page() const
{
    const httplib::Result res{ _client->Get("/html/update_video.html") };
    return client::format_page(res);
}

std::string Client::watch_video_page() const
{
    const httplib::Result res{ _client->Get("/html/watch_video.html") };
    return client::format_page(res);
}

std::vector<std::string> Client::admin_video_list() const
{
    const httplib::Result res{ _client->Get("/admin-video-list") };
    const std::string str_ids{ client::format_page(res) };
    return su::split(str_ids);
}

namespace client
{
    httplib::Result get(const std::string& path, const std::unique_ptr<httplib::Client>& client, const httplib::Params& param, const httplib::Headers& headers);
}

// from httplib.h, append_query_params and static constructor std::regex
// NOLINTNEXTLINE(bugprone-exception-escape)
inline httplib::Result client::get(const std::string& path, const std::unique_ptr<httplib::Client>& client, const httplib::Params& params, const httplib::Headers& headers)
{
    return client->Get(path, params, headers);
}

std::vector<std::string> Client::admin_video_list(const std::string& search) const
{
    const httplib::Headers headers{};
    const httplib::Params params{
        { "search", search }
    };
    const httplib::Result res{ client::get("/admin-video-list", _client, params, headers) };
    const std::string str_ids{ client::format_page(res) };
    return su::split(str_ids);
}

std::vector<std::string> Client::user_video_list(const std::string& user_id) const
{
    const httplib::Headers headers{
        { "user_id", user_id }
    };
    const httplib::Result res{ _client->Get("/user-video-list", headers) };
    const std::string str_ids{ client::format_page(res) };
    return su::split(str_ids);
}

std::vector<std::string> Client::user_video_list(const std::string& user_id, const std::string& search) const
{
    const httplib::Headers headers{
        { "user_id", user_id }
    };
    const httplib::Params params{
        { "search", search }
    };
    const httplib::Result res{ client::get("/user-video-list", _client, params, headers) };
    const std::string str_ids{ client::format_page(res) };
    return su::split(str_ids);
}

std::vector<std::string> Client::no_user_video_list() const
{
    const httplib::Result res{ _client->Get("/user-video-list") };
    const std::string str_ids{ client::format_page(res) };
    return su::split(str_ids);
}

std::vector<std::string> Client::no_user_video_list(const std::string& search) const
{
    const httplib::Headers headers{};
    const httplib::Params params{
        { "search", search }
    };
    const httplib::Result res{ client::get("/user-video-list", _client, params, headers) };
    const std::string str_ids{ client::format_page(res) };
    return su::split(str_ids);
}

std::string Client::video_title(const std::string& video_id) const
{
    const httplib::Result res{ _client->Get("/title/" + video_id) };
    return client::format_page(res);
}

int Client::video_views(const std::string& video_id) const
{
    const httplib::Result res{ _client->Get("/views/" + video_id) };
    return su::string_to_int(client::format_page(res));
}

bool Client::is_admin(const std::string& user_id) const
{
    const httplib::Headers headers{
        { "user_id", user_id }
    };
    const httplib::Result res{ _client->Get("/is-admin", headers) };
    return su::string_to_bool(client::format_page(res));
}

bool Client::is_super_admin(const std::string& user_id) const
{
    const httplib::Headers headers{
        { "user_id", user_id }
    };
    const httplib::Result res{ _client->Get("/is-super-admin", headers) };
    return su::string_to_bool(client::format_page(res));
}

bool Client::is_user(const std::string& user_id) const
{
    const httplib::Headers headers{
        { "user_id", user_id }
    };
    const httplib::Result res{ _client->Get("/is-user", headers) };
    return su::string_to_bool(client::format_page(res));
}

std::string Client::user_name(const std::string& user_id) const
{
    const httplib::Headers headers{
        { "user_id", user_id }
    };
    const httplib::Result res{ _client->Get("/user-name", headers) };
    return client::format_page(res);
}

std::string Client::user_id(const std::string& username) const
{
    const httplib::Headers headers{
        { "username", username }
    };
    const httplib::Result res{ _client->Get("/user-id", headers) };
    return client::format_page(res);
}

std::string Client::add_admin(const std::string& username) const
{
    const httplib::MultipartFormDataItems items{
        { .name = "username", .content = username, .filename = "", .content_type = "" }
    };
    const httplib::Result res{ _client->Post("/add-admin", items) };
    return client::format_page(res);
}

std::string Client::add_user(const std::string& username) const
{
    const httplib::MultipartFormDataItems items{
        { .name = "username", .content = username, .filename = "", .content_type = "" }
    };
    const httplib::Result res{ _client->Post("/add-user", items) };
    return client::format_page(res);
}

std::string Client::add_password(const std::string& user_id, const std::string& password) const
{
    const httplib::MultipartFormDataItems items{
        { .name = "user_id", .content = user_id, .filename = "", .content_type = "" },
        { .name = "password", .content = password, .filename = "", .content_type = "" }
    };
    const httplib::Result res{ _client->Post("/add-password", items) };
    return client::format_page(res);
}

void Client::update_username(const std::string& user_id, const std::string& username) const
{
    const httplib::MultipartFormDataItems items{
        { .name = "user_id", .content = user_id, .filename = "", .content_type = "" },
        { .name = "username", .content = username, .filename = "", .content_type = "" }
    };
    _client->Post("/update-username", items);
}

void Client::update_password(const std::string& user_id, const std::string& password) const
{
    const httplib::MultipartFormDataItems items{
        { .name = "user_id", .content = user_id, .filename = "", .content_type = "" },
        { .name = "password", .content = password, .filename = "", .content_type = "" }
    };
    _client->Post("/update-password", items);
}

void Client::reset_user(const std::string& user_id) const
{
    const httplib::MultipartFormDataItems items{
        { .name = "user_id", .content = user_id, .filename = "", .content_type = "" }
    };
    _client->Post("/reset-user", items);
}

void Client::delete_user(const std::string& user_id) const
{
    const httplib::MultipartFormDataItems items{
        { .name = "user_id", .content = user_id, .filename = "", .content_type = "" }
    };
    _client->Post("/delete-user", items);
}

bool Client::is_first_connection(const std::string& user_id) const
{
    const httplib::MultipartFormDataItems items{
        { .name = "user_id", .content = user_id, .filename = "", .content_type = "" }
    };
    const httplib::Result res{ _client->Post("/is-first-connection", items) };
    return su::string_to_bool(client::format_page(res));
}

bool Client::is_valid_user(const std::string& user_id, const std::string& password) const
{
    const httplib::MultipartFormDataItems items{
        { .name = "user_id", .content = user_id, .filename = "", .content_type = "" },
        { .name = "password", .content = password, .filename = "", .content_type = "" }
    };
    const httplib::Result res{ _client->Post("/is-valid-user", items) };
    return su::string_to_bool(client::format_page(res));
}

int Client::user_count() const
{
    const httplib::Result res{ _client->Get("/user-count") };
    return su::string_to_int(client::format_page(res));
}

int Client::group_count() const
{
    const httplib::Result res{ _client->Get("/group-count") };
    return su::string_to_int(client::format_page(res));
}

int Client::video_count() const
{
    const httplib::Result res{ _client->Get("/video-count") };
    return su::string_to_int(client::format_page(res));
}

int Client::view_count() const
{
    const httplib::Result res{ _client->Get("/view-count") };
    return su::string_to_int(client::format_page(res));
}

std::vector<std::string> Client::user_list() const
{
    const httplib::Result res{ _client->Get("/user-list") };
    const std::string str_user_ids{ client::format_page(res) };
    return su::split(str_user_ids);
}

std::vector<std::string> Client::admin_list() const
{
    const httplib::Result res{ _client->Get("/admin-list") };
    const std::string str_user_ids{ client::format_page(res) };
    return su::split(str_user_ids);
}

std::vector<std::string> Client::group_list() const
{
    const httplib::Result res{ _client->Get("/group-list") };
    const std::string str_group_ids{ client::format_page(res) };
    return su::split(str_group_ids);
}

std::string Client::group_name(const std::string& group_id) const
{
    const httplib::Headers headers{
        { "group_id", group_id }
    };
    const httplib::Result res{ _client->Get("/group-name", headers) };
    return client::format_page(res);
}

bool Client::group_exists(const std::string& name) const
{
    const httplib::Headers headers{
        { "name", name }
    };
    const httplib::Result res{ _client->Get("/group-exists", headers) };
    return su::string_to_bool(client::format_page(res));
}

std::string Client::add_group(const std::string& name, const std::vector<std::string>& group_user_ids) const
{
    const httplib::MultipartFormDataItems items{
        { .name = "name", .content = name, .filename = "", .content_type = "" },
        { .name = "user_ids", .content = su::join(group_user_ids), .filename = "", .content_type = "" }
    };
    const httplib::Result res{ _client->Post("/add-group", items) };
    return client::format_page(res);
}

void Client::update_group(const std::string& group_id, const std::string& name, const std::vector<std::string>& group_user_ids) const
{
    const httplib::MultipartFormDataItems items{
        { .name = "name", .content = name, .filename = "", .content_type = "" },
        { .name = "user_ids", .content = su::join(group_user_ids), .filename = "", .content_type = "" }
    };
    _client->Post("/update-group/" + group_id, items);
}

void Client::delete_group(const std::string& group_id) const
{
    _client->Post("/delete-group/" + group_id);
}

std::vector<std::string> Client::group_user_list(const std::string& group_id) const
{
    const httplib::Result res{ _client->Get("/group-user-list/" + group_id) }; // NOLINT(clang-analyzer-unix.BlockInCriticalSection): from httplib.h, why just here?
    const std::string str_users{ client::format_page(res) };
    return su::split(str_users);
}

std::string Client::add_video(const std::string& title, const std::string& content, const std::vector<std::string>& allowed_user_ids) const
{
    const httplib::MultipartFormDataItems items{
        { .name = "title", .content = title, .filename = "", .content_type = "" },
        { .name = "video", .content = content, .filename = "", .content_type = "" },
        { .name = "user_ids", .content = su::join(allowed_user_ids), .filename = "", .content_type = "" }
    };
    const httplib::Result res{ _client->Post("/add-video", items) };
    return client::format_page(res);
}

void Client::update_video(const std::string& video_id, const std::string& title, const std::vector<std::string>& allowed_user_ids) const
{
    const httplib::MultipartFormDataItems items{
        { .name = "title", .content = title, .filename = "", .content_type = "" },
        { .name = "user_ids", .content = su::join(allowed_user_ids), .filename = "", .content_type = "" }
    };
    _client->Post("/update-video/" + video_id, items);
}

void Client::delete_video(const std::string& video_id) const
{
    _client->Post("/delete-video/" + video_id);
}

void Client::increment_video_views(const std::string& video_id) const
{
    _client->Post("/increment-video-views/" + video_id);
}

std::string Client::video(const std::string& video_id, std::size_t offset, std::size_t length) const
{
    const httplib::Headers headers{
        { "Offset", su::int_to_string(static_cast<int>(offset)) },
        { "Length", su::int_to_string(static_cast<int>(length)) }
    };
    const httplib::Result res{ _client->Get("/video/" + video_id, headers) };
    // logging::debug{ "Video length: {}", video_content.size() };

    if (!res) {
        if (res.error() == httplib::Error::Canceled) {
            // The stream can be cancelled by the request or the user
            logging::info{ "Video stream cancelled: {} ({})", httplib::to_string(res.error()), static_cast<int>(res.error()) };
        } else {
            logging::error{ "Fail to get the video with error: {} ({})", httplib::to_string(res.error()), static_cast<int>(res.error()) };
        }
        return {};
    }

    if (res->status != httplib::StatusCode::OK_200 && res->status != httplib::StatusCode::PartialContent_206) {
        logging::error{ "Fail to get the video with error: {} ({})", httplib::status_message(res->status), res->status };
        return {};
    }

    return res->body;
}

int Client::video_size(const std::string& video_id) const
{
    const httplib::Result res{ _client->Get("/video-size/" + video_id) };
    const int video_size{ su::string_to_int(client::format_page(res)) };
    logging::debug{ "Video length: {}", video_size };
    return video_size;
}

std::string Client::thumbnail(const std::string& video_id) const
{
    std::string thumbnail_content;
    const httplib::Result res{
        _client->Get("/thumbnail/" + video_id,
                     [&thumbnail_content](const char* data, std::size_t data_length) -> bool {
                         thumbnail_content.append(data, data_length);
                         return true;
                     })
    };
    logging::debug{ "Thumbnail length: {}", thumbnail_content.size() };

    if (!res) {
        logging::error{ "Fail to get the thumbnail with error: {} ({})", httplib::to_string(res.error()), static_cast<int>(res.error()) };
        return {};
    }

    if (res->status != httplib::StatusCode::OK_200 && res->status != httplib::StatusCode::PartialContent_206) {
        logging::error{ "Fail to get the thumbnail with error: {} ({})", httplib::status_message(res->status), res->status };
        return {};
    }

    return thumbnail_content;
}

bool Client::has_video_right(const std::string& video_id) const
{
    const httplib::Result res{ _client->Get("/has-video-right/" + video_id) };
    return su::string_to_bool(client::format_page(res));
}

bool Client::has_video_right(const std::string& video_id, const std::string& user_id) const
{
    const httplib::Headers headers{
        { "user_id", user_id }
    };
    const httplib::Result res{ _client->Get("/has-video-right/" + video_id, headers) };
    return su::string_to_bool(client::format_page(res));
}

std::vector<std::string> Client::video_user_right_list(const std::string& video_id) const
{
    const httplib::Result res{ _client->Get("/video-user-right-list/" + video_id) }; // NOLINT(clang-analyzer-unix.BlockInCriticalSection): from httplib.h, why just here?
    const std::string str_rights{ client::format_page(res) };
    return su::split(str_rights);
}

inline std::string client::generic_error(int error, const std::string& message)
{
    return std::format(generic_error_html, "Error", error, message, CPPHTTPLIB_VERSION);
}

inline std::string client::format_page(const httplib::Result& res)
{
    if (!res)
        return generic_error(static_cast<int>(res.error()), httplib::to_string(res.error()));

    if (res->status != httplib::StatusCode::OK_200)
        return generic_error(res->status, httplib::status_message(res->status));

    return res->body;
}
