#include "server.h"

#include "client.h"
#include "confirmhandler.h"
#include "crypto.h"
#include "logging.h"
#include "servercommon.h"
#include "session.h"
#include "stringutils.h"

#include <httplib.h>
#include <inja.hpp>

#include <stacktrace>

namespace server
{
    constexpr std::string_view footer();
    constexpr std::size_t video_chunk_size();

    template <typename AlertType, std::size_t N>
        requires(std::is_enum_v<AlertType>)
    constexpr std::string_view alert_to_text(AlertType alert, const std::array<std::string_view, N>& alert_texts);

    void set_no_cache_headers(httplib::Response& res);

    void set_error_handler(httplib::Server& server, inja::Environment& env, const Client& client);
    void set_exception_handler(httplib::Server& server, inja::Environment& env);
    void set_logger(httplib::Server& server);

    void register_session(const httplib::Request& req, httplib::Response& res, Session& session, const std::string& user_id);

    bool is_logged(const httplib::Request& req, httplib::Response& res, const Session& session, const Client& client);
    bool is_logged_and_admin(const httplib::Request& req, httplib::Response& res, const Session& session, const Client& client);
    std::string connected_user_id(const httplib::Request& req, const Session& session);

    bool username_exists(const std::string& username, const Client& client);

    inja::json video_dict(const std::vector<std::string>& video_ids, const Client& client);

    std::vector<std::string> param_value_list(const httplib::Request& req, const std::string& param_name);
    std::vector<std::string> fields_list(const httplib::Request& req, const std::string& file_name);

    void video_service_worker_file(const httplib::Request& req, httplib::Response& res, const Client& client);
    void static_file(const httplib::Request& req, httplib::Response& res, const Client& client);

    void home(const httplib::Request& req, httplib::Response& res, inja::Environment& env, const Session& session, const Client& client);
    void dashboard(const httplib::Request& req, httplib::Response& res, inja::Environment& env, const Session& session, const Client& client);

    void login_get(const httplib::Request& req, httplib::Response& res, inja::Environment& env, const Session& session, const Client& client);
    void login_post(const httplib::Request& req, httplib::Response& res, inja::Environment& env, Session& session, const Client& client);
    void logout(const httplib::Request& req, httplib::Response& res, Session& session);

    void admin_list(const httplib::Request& req, httplib::Response& res, inja::Environment& env, const Session& session, const Client& client);
    void user_list(const httplib::Request& req, httplib::Response& res, inja::Environment& env, const Session& session, const Client& client);

    void add_user_get(const httplib::Request& req, httplib::Response& res, inja::Environment& env, const Session& session, const Client& client);
    void add_user_post(const httplib::Request& req, httplib::Response& res, inja::Environment& env, const Session& session, const Client& client);

    void add_password_get(const httplib::Request& req, httplib::Response& res, inja::Environment& env, const Session& session, const Client& client);
    void add_password_post(const httplib::Request& req, httplib::Response& res, inja::Environment& env, Session& session, const Client& client);

    void confirm_action(const httplib::Request& req, httplib::Response& res, inja::Environment& env, Session& session, const Client& client, const std::string& confirm_signal_str);
    void confirm(const httplib::Request& req, httplib::Response& res, ConfirmHandler& confirm_handler, Session& session, const Client& client);

    void update_user_admin(const httplib::Request& req, httplib::Response& res, inja::Environment& env, Session& session, const Client& client);
    void update_username_admin(const httplib::Request& req, httplib::Response& res, inja::Environment& env, ConfirmHandler& confirm_handler, Session& session, const Client& client);

    void reset_user(const httplib::Request& req, httplib::Response& res, inja::Environment& env, ConfirmHandler& confirm_handler, Session& session, const Client& client);
    void delete_user(const httplib::Request& req, httplib::Response& res, inja::Environment& env, ConfirmHandler& confirm_handler, Session& session, const Client& client);

    void update_user(const httplib::Request& req, httplib::Response& res, inja::Environment& env, Session& session, const Client& client);
    void update_username(const httplib::Request& req, httplib::Response& res, inja::Environment& env, ConfirmHandler& confirm_handler, Session& session, const Client& client);
    void update_password(const httplib::Request& req, httplib::Response& res, inja::Environment& env, ConfirmHandler& confirm_handler, Session& session, const Client& client);

    void group_list(const httplib::Request& req, httplib::Response& res, inja::Environment& env, const Session& session, const Client& client);
    void add_group_get(const httplib::Request& req, httplib::Response& res, inja::Environment& env, const Session& session, const Client& client);
    void add_group_post(const httplib::Request& req, httplib::Response& res, inja::Environment& env, const Session& session, const Client& client);
    void update_group_get(const httplib::Request& req, httplib::Response& res, inja::Environment& env, Session& session, const Client& client);
    void update_group_post(const httplib::Request& req, httplib::Response& res, inja::Environment& env, ConfirmHandler& confirm_handler, Session& session, const Client& client);
    void delete_group(const httplib::Request& req, httplib::Response& res, inja::Environment& env, ConfirmHandler& confirm_handler, Session& session, const Client& client);

    void video_list(const httplib::Request& req, httplib::Response& res, inja::Environment& env, const Session& session, const Client& client);
    void add_video_get(const httplib::Request& req, httplib::Response& res, inja::Environment& env, const Session& session, const Client& client);
    void add_video_post(const httplib::Request& req, httplib::Response& res, inja::Environment& env, const Session& session, const Client& client);
    void update_video_get(const httplib::Request& req, httplib::Response& res, inja::Environment& env, Session& session, const Client& client);
    void update_video_post(const httplib::Request& req, httplib::Response& res, inja::Environment& env, ConfirmHandler& confirm_handler, Session& session, const Client& client);
    void delete_video(const httplib::Request& req, httplib::Response& res, inja::Environment& env, ConfirmHandler& confirm_handler, Session& session, const Client& client);

    void download_video(const httplib::Request& req, httplib::Response& res, const Session& session, const Client& client);

    void watch_video(const httplib::Request& req, httplib::Response& res, inja::Environment& env, Session& session, const Client& client);
    void video_playlist(const httplib::Request& req, httplib::Response& res, const Session& session, const Client& client);
    void video_segment(const httplib::Request& req, httplib::Response& res, const Session& session, const Client& client);
    void increment_video_views(const httplib::Request& req, httplib::Response& res, const Session& session, const Client& client);
    void thumbnail(const httplib::Request& req, httplib::Response& res, const Session& session, const Client& client);
}

int server::start()
{
    const Client client;
    inja::Environment env;
    Session session;
    ConfirmHandler confirm_handler;
    httplib::Server server;

    std::string&& website_name{ sc::get_env("WEBSITE_NAME", "Simple Video CMS") };
    env.add_callback("website_name", 0, [website_name](const inja::Arguments&) -> std::string {
        return website_name;
    });

    std::string&& icon_path{ sc::get_env("ICON_PATH", "/static/img/icon.svg") };
    env.add_callback("icon", 0, [icon_path](const inja::Arguments&) -> std::string {
        return icon_path;
    });

    env.add_callback("footer", 0, [](const inja::Arguments&) -> std::string_view {
        return footer();
    });

    env.add_callback("empty", 1, [](const inja::Arguments& args) -> bool {
        const inja::json* const val{ args.at(0) };
        if (val->is_string()) {
            return val->get_ref<const inja::json::string_t&>().empty();
        }
        return val->empty();
    });

    set_error_handler(server, env, client);
    set_exception_handler(server, env);
    set_logger(server);

    server.set_post_routing_handler([](const httplib::Request& req, httplib::Response& res) {
        if (!req.path.contains("static")) {
            set_no_cache_headers(res);
        }
    });

    server
        .Get("/static/js/videoserviceworker.js", sc::serve(video_service_worker_file, std::cref(client)))
        .Get(sc::static_regexp_path(), sc::serve(static_file, std::cref(client)))

        .Get("/", sc::serve(home, std::ref(env), std::cref(session), std::cref(client)))
        .Get("/dashboard", sc::serve(dashboard, std::ref(env), std::cref(session), std::cref(client)))

        .Get("/login", sc::serve(login_get, std::ref(env), std::cref(session), std::cref(client)))
        .Post("/login", sc::serve(login_post, std::ref(env), std::ref(session), std::cref(client)))

        .Get("/logout", sc::serve(logout, std::ref(session)))

        .Get("/admin-list", sc::serve(admin_list, std::ref(env), std::cref(session), std::cref(client)))
        .Get("/user-list", sc::serve(user_list, std::ref(env), std::cref(session), std::cref(client)))

        .Get("/add-user", sc::serve(add_user_get, std::ref(env), std::cref(session), std::cref(client)))
        .Post("/add-user", sc::serve(add_user_post, std::ref(env), std::cref(session), std::cref(client)))

        .Get("/add-password", sc::serve(add_password_get, std::ref(env), std::cref(session), std::cref(client)))
        .Get("/add-password/:username", sc::serve(add_password_get, std::ref(env), std::cref(session), std::cref(client)))
        .Post("/add-password", sc::serve(add_password_post, std::ref(env), std::ref(session), std::cref(client)))

        .Post("/confirm", sc::serve(confirm, std::ref(confirm_handler), std::ref(session), std::cref(client)))

        .Get("/update-user-admin/:user_id", sc::serve(update_user_admin, std::ref(env), std::ref(session), std::cref(client)))
        .Post("/update-username-admin/:user_id", sc::serve(update_username_admin, std::ref(env), std::ref(confirm_handler), std::ref(session), std::cref(client)))

        .Get("/reset-user/:user_id", sc::serve(reset_user, std::ref(env), std::ref(confirm_handler), std::ref(session), std::cref(client)))
        .Get("/delete-user/:user_id", sc::serve(delete_user, std::ref(env), std::ref(confirm_handler), std::ref(session), std::cref(client)))

        .Get("/update-user", sc::serve(update_user, std::ref(env), std::ref(session), std::cref(client)))
        .Post("/update-username", sc::serve(update_username, std::ref(env), std::ref(confirm_handler), std::ref(session), std::cref(client)))
        .Post("/update-password", sc::serve(update_password, std::ref(env), std::ref(confirm_handler), std::ref(session), std::cref(client)))

        .Get("/group-list", sc::serve(group_list, std::ref(env), std::cref(session), std::cref(client)))

        .Get("/add-group", sc::serve(add_group_get, std::ref(env), std::cref(session), std::cref(client)))
        .Post("/add-group", sc::serve(add_group_post, std::ref(env), std::cref(session), std::cref(client)))
        .Get("/update-group/:group_id", sc::serve(update_group_get, std::ref(env), std::ref(session), std::cref(client)))
        .Post("/update-group/:group_id", sc::serve(update_group_post, std::ref(env), std::ref(confirm_handler), std::ref(session), std::cref(client)))
        .Get("/delete-group/:group_id", sc::serve(delete_group, std::ref(env), std::ref(confirm_handler), std::ref(session), std::cref(client)))

        .Get("/video-list", sc::serve(video_list, std::ref(env), std::cref(session), std::cref(client)))

        .Get("/add-video", sc::serve(add_video_get, std::ref(env), std::cref(session), std::cref(client)))
        .Post("/add-video", sc::serve(add_video_post, std::ref(env), std::cref(session), std::cref(client)))
        .Get("/update-video/:video_id", sc::serve(update_video_get, std::ref(env), std::ref(session), std::cref(client)))
        .Post("/update-video/:video_id", sc::serve(update_video_post, std::ref(env), std::ref(confirm_handler), std::ref(session), std::cref(client)))
        .Get("/delete-video/:video_id", sc::serve(delete_video, std::ref(env), std::ref(confirm_handler), std::ref(session), std::cref(client)))

        .Get("/download-video/:video_id", sc::serve(download_video, std::cref(session), std::cref(client)))

        .Get("/watch-video/:video_id", sc::serve(watch_video, std::ref(env), std::ref(session), std::cref(client)))
        .Get("/video/:video_id/playlist", sc::serve(video_playlist, std::cref(session), std::cref(client)))
        .Get("/video/:video_id/:segment", sc::serve(video_segment, std::cref(session), std::cref(client)))
        .Post("/increment_video_views/:video_id", sc::serve(increment_video_views, std::cref(session), std::cref(client)))
        .Get("/thumbnail/:video_id", sc::serve(thumbnail, std::cref(session), std::cref(client)));

    const std::string host{ sc::get_env("FRONT_HOST", "0.0.0.0") };
    const int port{ su::string_to_int(sc::get_env("FRONT_PORT", "8080")) };
    logging::info{ "Serving HTTP on {0} port {1} ...", host, port };
    return (server.listen(host, port) ? EXIT_SUCCESS : EXIT_FAILURE);
}

constexpr std::string_view server::footer()
{
    using namespace std::literals::string_view_literals;
    return R"(
    <div class="footer pure-g">
      <div class="pure-u-1 pure-u-sm-1-2">
        <p class="legal-license">This site is built with ❤️ using
          <a href="https://pure-css.github.io/">Pure CSS</a>,
          <a href="https://videojs.com/">Video.js</a>,
          <a href="https://www.ffmpeg.org/">FFmpeg</a>,
          <a href="https://sqliteorm.com/">SQLite ORM</a> and
          many awesome <a href="https://gitlab.devau.co/theo/simple-video-cms/-/blob/prod/common/third-party/Readme.md">c++ libraries</a>.<br>
          All code on this site is licensed under the
          <a href="https://gitlab.devau.co/theo/simple-video-cms/-/blob/prod/LICENSE">GPLv3</a> unless otherwise stated.
        </p>
      </div>
      <div class="pure-u-1 pure-u-sm-1-2">
        <br>
        <p class="legal-link"><a href="https://gitlab.devau.co/theo/simple-video-cms">Open Source Project</a></p>
      </div>
    </div>
    )"sv;
}

constexpr std::size_t server::video_chunk_size()
{
    constexpr unsigned long binary_prefix{ 1024UL };
    constexpr unsigned long chunk_factor{ 10UL };
    return static_cast<std::size_t>(chunk_factor * binary_prefix);
}

template <typename AlertType, std::size_t N>
    requires(std::is_enum_v<AlertType>)
constexpr std::string_view server::alert_to_text(AlertType alert, const std::array<std::string_view, N>& alert_texts)
{
    return alert_texts.at(std::to_underlying(alert));
}

inline void server::set_no_cache_headers(httplib::Response& res)
{
    res.set_header("Last-Modified", logging::time_local());
    res.set_header("Cache-Control", "no-store, no-cache, must-revalidate, post-check=0, pre-check=0, max-age=0");
    res.set_header("Pragma", "no-cache");
    res.set_header("Expires", "-1");
}

inline void server::set_error_handler(httplib::Server& server, inja::Environment& env, const Client& client)
{
    server.set_error_handler([&env, &client](const httplib::Request& /*req*/, httplib::Response& res) {
        std::string body;

        switch (res.status) {
            case httplib::StatusCode::NotFound_404:
                body = client.error_page_404();
                break;

            case httplib::StatusCode::Forbidden_403:
                body = client.error_page_403();
                break;

            default:
                body = Client::generic_error(res.status, httplib::status_message(res.status));
        }

        body = env.render(body, {});
        res.set_content(body, "text/html");
    });
}

inline void server::set_exception_handler(httplib::Server& server, inja::Environment& env)
{
    server.set_exception_handler([&env](const httplib::Request& /*req*/, httplib::Response& res, std::exception_ptr ep) {
        constexpr int error_code{ httplib::StatusCode::InternalServerError_500 };
        std::string message;
        try {
            std::rethrow_exception(std::move(ep));
        } catch (const std::exception& e) {
            message = e.what();
        } catch (...) {
            message = "Unknown Exception";
        }

        logging::error{ std::to_string(std::stacktrace::current()) };
        logging::error{ message };

        std::string body{ Client::generic_error(error_code, message) };
        body = env.render(body, {});

        res.set_content(body, "text/html");
        res.status = error_code;
    });
}

inline void server::set_logger(httplib::Server& server)
{
    server.set_logger([](const httplib::Request& req, const httplib::Response& res) {
        logging::raw_log(sc::log(req, res));
    });
}

inline void server::register_session(const httplib::Request& req, httplib::Response& res, Session& session, const std::string& user_id)
{
    const std::string session_id{ session.create_session(user_id) };
    res.set_header("Set-Cookie", session.insert_session_id_to_cookie(req.get_header_value("Host"), session_id));
}

inline bool server::is_logged(const httplib::Request& req, httplib::Response& res, const Session& session, const Client& /*client*/)
{
    const std::string cookie{ req.get_header_value("Cookie") };
    const std::string session_id{ Session::extract_session_id_from_cookie(cookie) };
    if (!session.is_valid_session(session_id)) {
        res.set_redirect("/login");
        return false;
    }

    return true;
}

inline bool server::is_logged_and_admin(const httplib::Request& req, httplib::Response& res, const Session& session, const Client& client)
{
    const std::string cookie{ req.get_header_value("Cookie") };
    const std::string session_id{ Session::extract_session_id_from_cookie(cookie) };
    if (!session.is_valid_session(session_id)) {
        res.set_redirect("/login");
        return false;
    }

    if (!client.is_admin(session.user_from_session(session_id))) {
        res.status = httplib::StatusCode::Forbidden_403;
        return false;
    }

    return true;
}

inline std::string server::connected_user_id(const httplib::Request& req, const Session& session)
{
    const std::string cookie{ req.get_header_value("Cookie") };
    const std::string session_id{ Session::extract_session_id_from_cookie(cookie) };
    return session.user_from_session(session_id);
}

inline bool server::username_exists(const std::string& username, const Client& client)
{
    const std::string tested_user_id{ client.user_id(username) };
    const bool is_user{ client.is_user(tested_user_id) };
    const bool is_admin{ client.is_admin(tested_user_id) };
    return (is_user || is_admin);
}

inline inja::json server::video_dict(const std::vector<std::string>& video_ids, const Client& client)
{
    inja::json video_dict;
    for (const std::string& video_id : video_ids) {
        const std::string title{ client.video_title(video_id) };
        const int views{ client.video_views(video_id) };
        video_dict.emplace_back(inja::json::object({ { "id", video_id }, { "title", title }, { "views", views } }));
    }
    return video_dict;
}

inline std::vector<std::string> server::param_value_list(const httplib::Request& req, const std::string& param_name)
{
    const std::string key{ param_name + "[]" };
    const std::size_t param_count{ req.get_param_value_count(key) };
    std::vector<std::string> param_list(param_count);
    for (std::size_t i{ 0 }; i < param_count; ++i) {
        param_list[i] = req.get_param_value(key, i);
    }
    return param_list;
}

inline std::vector<std::string> server::fields_list(const httplib::Request& req, const std::string& file_name)
{
    const std::string key{ file_name + "[]" };
    const std::vector file_items{ req.form.get_fields(key) };
    return file_items;
}

inline void server::video_service_worker_file(const httplib::Request& /*req*/, httplib::Response& res, const Client& client)
{
    static const std::string file{ "js/videoserviceworker.js" };
    const auto [content, content_type]{ client.static_file(file) };
    res.set_header("Service-Worker-Allowed", "/");
    res.set_content(content, content_type);
}

inline void server::static_file(const httplib::Request& req, httplib::Response& res, const Client& client)
{
    const std::string file{ req.matches[1] };
    const auto [content, content_type]{ client.static_file(file) };
    res.set_content(content, content_type);
}

inline void server::home(const httplib::Request& req, httplib::Response& res, inja::Environment& env, const Session& session, const Client& client)
{
    const std::string cookie{ req.get_header_value("Cookie") };
    const std::string session_id{ Session::extract_session_id_from_cookie(cookie) };
    const bool is_logged{ session.is_valid_session(session_id) };
    const std::string& user_id{ session.user_from_session(session_id) };
    const bool is_admin{ is_logged && client.is_admin(user_id) };

    std::vector<std::string> video_ids;
    if (req.has_param("search")) {
        const std::string search{ req.get_param_value("search") };
        if (is_admin)
            video_ids = client.admin_video_list(search);
        else if (is_logged)
            video_ids = client.user_video_list(user_id, search);
        else
            video_ids = client.no_user_video_list(search);
    } else {
        if (is_admin)
            video_ids = client.admin_video_list();
        else if (is_logged)
            video_ids = client.user_video_list(user_id);
        else
            video_ids = client.no_user_video_list();
    }

    const inja::json data{
        { "is_logged", is_logged },
        { "is_admin", is_admin },
        { "video_dict", video_dict(video_ids, client) }
    };
    logging::debug{ data.dump() };

    const std::string body{ env.render(client.home_page(), data) }; // NOLINT(clang-analyzer-core.StackAddressEscape): in inja.hpp Parser::parse
    res.set_content(body, "text/html");
}

inline void server::dashboard(const httplib::Request& req, httplib::Response& res, inja::Environment& env, const Session& session, const Client& client)
{
    if (!is_logged_and_admin(req, res, session, client)) // NOLINT(clang-analyzer-core.StackAddressEscape): in inja.hpp Parser::parse
        return;

    const inja::json data{
        { "user_count", client.user_count() },
        { "group_count", client.group_count() },
        { "video_count", client.video_count() },
        { "view_count", client.view_count() }
    };
    logging::debug{ data.dump() };

    const std::string body{ env.render(client.dashboard_page(), data) }; // NOLINT(clang-analyzer-core.StackAddressEscape): in inja.hpp Parser::parse
    res.set_content(body, "text/html");
}

namespace server
{
    enum class AlertLogin : std::uint8_t
    {
        no_alert,
        unknown_username,
        invalid_password
    };
    using namespace std::literals::string_view_literals;
    static constexpr std::array alert_login_texts{ ""sv, "Unknown username"sv, "Invalid password"sv };

    void set_login_content(httplib::Response& res, inja::Environment& env, const Client& client, AlertLogin alert);
    void login_redirect(const httplib::Request& req, httplib::Response& res, const Session& session);
}

inline void server::set_login_content(httplib::Response& res, inja::Environment& env, const Client& client, AlertLogin alert)
{
    const inja::json data{
        { "alert", alert_to_text(alert, alert_login_texts) }
    };
    logging::debug{ data.dump() };

    const std::string body{ env.render(client.login_page(), data) }; // NOLINT(clang-analyzer-core.StackAddressEscape): in inja.hpp Parser::parse
    res.set_content(body, "text/html");
}

inline void server::login_redirect(const httplib::Request& req, httplib::Response& res, const Session& session)
{
    const std::string cookie{ req.get_header_value("Cookie") };
    const std::string session_id{ Session::extract_session_id_from_cookie(cookie) };
    if (!session.is_not_logged_session(session_id)) {
        res.set_redirect("/");
        return;
    }

    const std::string& video_id{ session.value_from_session(session_id, "video_id") };
    if (video_id.empty()) {
        res.set_redirect("/");
        return;
    }

    res.set_redirect("watch-video/" + video_id);
}

inline void server::login_get(const httplib::Request& req, httplib::Response& res, inja::Environment& env, const Session& session, const Client& client)
{
    const std::string cookie{ req.get_header_value("Cookie") };
    if (session.is_valid_session_from_cookie(cookie)) {
        login_redirect(req, res, session);
        return;
    }

    set_login_content(res, env, client, AlertLogin::no_alert);
}

inline void server::login_post(const httplib::Request& req, httplib::Response& res, inja::Environment& env, Session& session, const Client& client)
{
    std::string username{ req.get_param_value("username") };
    su::trim(username);
    su::lower(username);

    if (username.empty()) {
        // first connection link clicked maybe
        res.set_redirect("/add-password");
        return;
    }

    if (!username_exists(username, client)) {
        set_login_content(res, env, client, AlertLogin::unknown_username);
        return;
    }

    const std::string user_id{ client.user_id(username) };
    const bool is_first_connection{ client.is_first_connection(user_id) };
    if (is_first_connection) {
        res.set_redirect("/add-password/" + username);
        return;
    }

    const std::string password{ crypto::sha512(req.get_param_value("password")) };
    if (password.empty()) {
        set_login_content(res, env, client, AlertLogin::invalid_password);
        return;
    }

    const bool is_valid_user{ client.is_valid_user(user_id, password) };
    if (!is_valid_user) {
        set_login_content(res, env, client, AlertLogin::invalid_password);
        return;
    }

    register_session(req, res, session, user_id);
    login_redirect(req, res, session);
}

inline void server::logout(const httplib::Request& req, httplib::Response& res, Session& session)
{
    const std::string cookie{ req.get_header_value("Cookie") };
    const std::string session_id{ Session::extract_session_id_from_cookie(cookie) };
    session.remove_session(session_id);
    res.set_redirect("/");
}

inline void server::admin_list(const httplib::Request& req, httplib::Response& res, inja::Environment& env, const Session& session, const Client& client)
{
    if (!is_logged_and_admin(req, res, session, client)) // NOLINT(clang-analyzer-core.StackAddressEscape): in inja.hpp Parser::parse
        return;

    const std::vector<std::string> admin_list{ client.admin_list() };
    inja::json admin_dict = inja::json::array();
    for (const std::string& user_id : admin_list) {
        const bool is_super_admin{ client.is_super_admin(user_id) };
        const inja::json admin = {
            { "id", user_id },
            { "name", client.user_name(user_id) },
            { "is_super_admin", is_super_admin },
            { "is_first_connection", client.is_first_connection(user_id) }
        };
        admin_dict += admin;
    }

    const inja::json data{ { "admin_dict", admin_dict } };
    logging::debug{ data.dump() };

    const std::string body{ env.render(client.admin_list_page(), data) }; // NOLINT(clang-analyzer-core.StackAddressEscape): in inja.hpp Parser::parse
    res.set_content(body, "text/html");
}

inline void server::user_list(const httplib::Request& req, httplib::Response& res, inja::Environment& env, const Session& session, const Client& client)
{
    if (!is_logged_and_admin(req, res, session, client)) // NOLINT(clang-analyzer-core.StackAddressEscape): in inja.hpp Parser::parse
        return;

    const std::vector<std::string> user_list{ client.user_list() };
    inja::json user_dict = inja::json::array();
    for (const std::string& user_id : user_list) {
        const std::vector user_groups{ client.user_group_list(user_id) };
        std::vector<std::string> group_list(user_groups.size());
        std::ranges::transform(user_groups, group_list.begin(),
                               [&](const std::string& group_id) -> std::string {
                                   return client.group_name(group_id);
                               });
        const inja::json user = {
            { "id", user_id },
            { "name", client.user_name(user_id) },
            { "group_list", group_list },
            { "is_first_connection", client.is_first_connection(user_id) }
        };
        user_dict += user;
    }

    const inja::json data{ { "user_dict", user_dict } };
    logging::debug{ data.dump() };

    const std::string body{ env.render(client.user_list_page(), data) }; // NOLINT(clang-analyzer-core.StackAddressEscape): in inja.hpp Parser::parse
    res.set_content(body, "text/html");
}

namespace server
{
    enum class AlertAddUser : std::uint8_t
    {
        no_alert,
        invalid_username
    };
    using namespace std::literals::string_view_literals;
    static constexpr std::array alert_add_user_texts{ ""sv, "Username already taken"sv };

    void set_add_user_content(httplib::Response& res, inja::Environment& env, const Client& client, bool is_admin, AlertAddUser alert);
}

inline void server::set_add_user_content(httplib::Response& res, inja::Environment& env, const Client& client, bool is_admin, AlertAddUser alert)
{
    inja::json data{
        { "is_admin", is_admin },
        { "alert", alert_to_text(alert, alert_add_user_texts) }
    };

    if (!is_admin) {
        const std::vector<std::string> group_list{ client.group_list() };
        inja::json group_dict = inja::json::array();
        for (const std::string& group_id : group_list) {
            const inja::json group{ { "id", group_id }, { "name", client.group_name(group_id) } };
            group_dict += group;
        }
        data["group_dict"] = group_dict;
    }

    logging::debug{ data.dump() };

    const std::string body{ env.render(client.add_user_page(), data) }; // NOLINT(clang-analyzer-core.StackAddressEscape): in inja.hpp Parser::parse
    res.set_content(body, "text/html");
}

inline void server::add_user_get(const httplib::Request& req, httplib::Response& res, inja::Environment& env, const Session& session, const Client& client)
{
    if (!is_logged_and_admin(req, res, session, client)) // NOLINT(clang-analyzer-core.StackAddressEscape): in inja.hpp Parser::parse
        return;

    const bool is_admin{ su::string_to_bool(req.get_param_value("is_admin")) };
    set_add_user_content(res, env, client, is_admin, AlertAddUser::no_alert);
}

inline void server::add_user_post(const httplib::Request& req, httplib::Response& res, inja::Environment& env, const Session& session, const Client& client)
{
    if (!is_logged_and_admin(req, res, session, client)) // NOLINT(clang-analyzer-core.StackAddressEscape): in inja.hpp Parser::parse
        return;

    const bool is_admin{ su::string_to_bool(req.get_param_value("is_admin")) };

    std::string username{ req.get_param_value("username") };
    su::trim(username);
    su::lower(username);

    const std::string creator_user_id{ connected_user_id(req, session) };

    if (username_exists(username, client)) {
        set_add_user_content(res, env, client, is_admin, AlertAddUser::invalid_username);
    } else if (is_admin) {
        const std::string user_id{ client.add_admin(username) };
        res.set_redirect("/admin-list");
        logging::info{ "Admin {} created by {}", user_id, creator_user_id };
    } else {
        const std::vector group_ids{ param_value_list(req, "group_ids") };
        const std::string user_id{ client.add_user(username, group_ids) };
        res.set_redirect("/user-list");
        logging::info{ "User {} created by {}", user_id, creator_user_id };
    }
}

namespace server
{
    enum class AlertAddPassword : std::uint8_t
    {
        no_alert,
        unknown_username,
        invalid_password,
        password_not_match,
        password_set,
        invalid_user
    };
    using namespace std::literals::string_view_literals;
    static constexpr std::array alert_add_password_texts{ ""sv, "Unknown username"sv, "Invalid password"sv, "Passwords do not match"sv, "Password already set"sv, "Invalid user"sv };

    void set_add_password_content(httplib::Response& res, inja::Environment& env, const Client& client, const std::string& username, AlertAddPassword alert);
}

inline void server::set_add_password_content(httplib::Response& res, inja::Environment& env, const Client& client, const std::string& username, AlertAddPassword alert)
{
    const inja::json data{
        { "username", username },
        { "alert", alert_to_text(alert, alert_add_password_texts) }
    };
    logging::debug{ data.dump() };
    const std::string body{ env.render(client.add_password_page(), data) }; // NOLINT(clang-analyzer-core.StackAddressEscape): in inja.hpp Parser::parse
    res.set_content(body, "text/html");
}

inline void server::add_password_get(const httplib::Request& req, httplib::Response& res, inja::Environment& env, const Session& session, const Client& client)
{
    const std::string cookie{ req.get_header_value("Cookie") };
    if (session.is_valid_session_from_cookie(cookie)) {
        res.set_redirect("/");
        return;
    }

    if (req.path_params.contains("username")) {
        const std::string username{ req.path_params.at("username") };
        set_add_password_content(res, env, client, username, AlertAddPassword::no_alert);
    } else {
        set_add_password_content(res, env, client, {}, AlertAddPassword::no_alert);
    }
}

inline void server::add_password_post(const httplib::Request& req, httplib::Response& res, inja::Environment& env, Session& session, const Client& client)
{
    const std::string cookie{ req.get_header_value("Cookie") };
    if (session.is_valid_session_from_cookie(cookie)) {
        res.set_redirect("/");
        return;
    }

    std::string username{ req.get_param_value("username") };
    su::trim(username);
    su::lower(username);

    if (!username_exists(username, client)) {
        set_add_password_content(res, env, client, {}, AlertAddPassword::unknown_username);
        return;
    }

    const std::string user_id{ client.user_id(username) };
    const bool is_first_connection{ client.is_first_connection(user_id) };
    if (!is_first_connection) {
        set_add_password_content(res, env, client, {}, AlertAddPassword::password_set);
        return;
    }

    const std::string password{ crypto::sha512(req.get_param_value("password")) };
    if (password.empty()) {
        set_add_password_content(res, env, client, username, AlertAddPassword::invalid_password);
        return;
    }

    const std::string confirm_password{ crypto::sha512(req.get_param_value("confirm-password")) };
    if (confirm_password != password) {
        set_add_password_content(res, env, client, username, AlertAddPassword::password_not_match);
        return;
    }

    const std::string user_id_check{ client.add_password(user_id, password) };
    if (user_id != user_id_check) {
        logging::error{ "Trying to add password with an invalid user" };
        set_add_password_content(res, env, client, {}, AlertAddPassword::invalid_user);
        return;
    }

    logging::info{ "User password updated by {}", user_id };

    register_session(req, res, session, user_id);
    res.set_redirect("/");
}

namespace server
{
    constexpr const char* session_confirm_key() { return "Confirm-Signal"; }
}

inline void server::confirm_action(const httplib::Request& req, httplib::Response& res, inja::Environment& env, Session& session, const Client& client, const std::string& confirm_signal_str)
{
    if (!is_logged_and_admin(req, res, session, client))
        return;

    const std::string cookie{ req.get_header_value("Cookie") };
    const std::string session_id{ Session::extract_session_id_from_cookie(cookie) };
    session.insert_value_from_session(session_id, session_confirm_key(), confirm_signal_str);

    const std::string body{ env.render(client.confirm_action_page(), {}) };
    res.set_content(body, "text/html");
}

inline void server::confirm(const httplib::Request& req, httplib::Response& res, ConfirmHandler& confirm_handler, Session& session, const Client& client)
{
    if (!is_logged_and_admin(req, res, session, client))
        return;

    const std::string cookie{ req.get_header_value("Cookie") };
    const std::string session_id{ Session::extract_session_id_from_cookie(cookie) };
    const std::string& confirm_signal_str{ session.value_from_session(session_id, session_confirm_key()) };

    const bool confirm{ su::string_to_bool(req.get_param_value("response")) };
    confirm_handler.confirm(res, confirm_signal_str, confirm);

    session.remove_value_from_session(session_id, session_confirm_key());
}

namespace server
{
    enum class AlertUpdateUserAdmin : std::uint8_t
    {
        no_alert,
        invalid_username
    };
    using namespace std::literals::string_view_literals;
    static constexpr std::array alert_update_user_admin_texts{ ""sv, "Username already taken"sv };

    void set_update_user_admin_content(httplib::Response& res, inja::Environment& env, const Client& client,
                                       const std::string& user_id, bool is_admin, AlertUpdateUserAdmin alert);
}

inline void server::set_update_user_admin_content(httplib::Response& res, inja::Environment& env, const Client& client,
                                                  const std::string& user_id, bool is_admin, AlertUpdateUserAdmin alert)
{
    inja::json data{
        { "user", { { "id", user_id }, { "name", client.user_name(user_id) } } },
        { "is_admin", is_admin },
        { "alert", alert_to_text(alert, alert_update_user_admin_texts) }
    };

    if (!is_admin) {
        const std::vector group_list{ client.group_list() };
        const std::vector user_group_list{ client.user_group_list(user_id) };

        inja::json group_dict = inja::json::array();
        for (const std::string& group_id : group_list) {
            const bool selected{ std::ranges::find(user_group_list, group_id) != user_group_list.cend() };
            const inja::json group{ { "id", group_id }, { "name", client.group_name(group_id) }, { "selected", selected } };
            group_dict += group;
        }
        data["group_dict"] = group_dict;
    }

    logging::debug{ data.dump() };

    const std::string body{ env.render(client.update_user_admin_page(), data) }; // NOLINT(clang-analyzer-core.StackAddressEscape): in inja.hpp Parser::parse
    res.set_content(body, "text/html");
}

inline void server::update_user_admin(const httplib::Request& req, httplib::Response& res, inja::Environment& env, Session& session, const Client& client)
{
    if (!is_logged_and_admin(req, res, session, client)) // NOLINT(clang-analyzer-core.StackAddressEscape): in inja.hpp Parser::parse
        return;

    const std::string user_id{ req.path_params.at("user_id") };
    const bool is_admin{ su::string_to_bool(req.get_param_value("is_admin")) };
    set_update_user_admin_content(res, env, client, user_id, is_admin, AlertUpdateUserAdmin::no_alert);
}

inline void server::update_username_admin(const httplib::Request& req, httplib::Response& res, inja::Environment& env, ConfirmHandler& confirm_handler, Session& session, const Client& client)
{
    if (!is_logged_and_admin(req, res, session, client)) // NOLINT(clang-analyzer-core.StackAddressEscape): in inja.hpp Parser::parse
        return;

    const bool is_admin{ su::string_to_bool(req.get_param_value("is_admin")) };

    const std::string updater_user_id{ connected_user_id(req, session) };
    const std::string user_id{ req.path_params.at("user_id") };

    std::string username{ req.get_param_value("username") };
    su::trim(username);
    su::lower(username);

    if (username != client.user_name(user_id) && username_exists(username, client)) {
        set_update_user_admin_content(res, env, client, user_id, is_admin, AlertUpdateUserAdmin::invalid_username);
        return;
    }

    const std::vector group_ids{ param_value_list(req, "group_ids") };

    const auto update_action{ [user_id, username, is_admin, group_ids, updater_user_id, &client] {
        client.update_username(user_id, username);
        if (!is_admin)
            client.update_user_groups(user_id, group_ids);
        const std::string user_type{ is_admin ? "Admin" : "User" };
        logging::info{ "{} name {} updated by {}", user_type, user_id, updater_user_id };
    } };

    const auto redirect_action{ [is_admin](httplib::Response& res) {
        if (is_admin)
            res.set_redirect("/admin-list");
        else
            res.set_redirect("/user-list");
    } };

    const std::string signal_str{
        confirm_handler.create()
            ->on_confirm([update_action, redirect_action](httplib::Response& res) {
                update_action();
                redirect_action(res);
            })
            .on_deny([redirect_action](httplib::Response& res) {
                redirect_action(res);
            })
            .to_string()
    };

    confirm_action(req, res, env, session, client, signal_str);
}

inline void server::reset_user(const httplib::Request& req, httplib::Response& res, inja::Environment& env, ConfirmHandler& confirm_handler, Session& session, const Client& client)
{
    if (!is_logged_and_admin(req, res, session, client))
        return;

    const std::string user_id{ req.path_params.at("user_id") };
    const bool is_admin{ su::string_to_bool(req.get_param_value("is_admin")) };
    logging::debug{ "Reset {} {}", user_id, client.user_name(user_id) };

    const std::string suppressor_user_id{ connected_user_id(req, session) };

    const auto reset_action{ [user_id, is_admin, suppressor_user_id, &client] {
        client.reset_user(user_id);
        const std::string user_type{ is_admin ? "Admin" : "User" };
        logging::info{ "{} {} reset by {}", user_type, user_id, suppressor_user_id };
    } };

    const auto redirect_action{ [is_admin](httplib::Response& res) {
        if (is_admin)
            res.set_redirect("/admin-list");
        else
            res.set_redirect("/user-list");
    } };

    const std::string signal_str{
        confirm_handler.create()
            ->on_confirm([reset_action, redirect_action](httplib::Response& res) {
                reset_action();
                redirect_action(res);
            })
            .on_deny([redirect_action](httplib::Response& res) {
                redirect_action(res);
            })
            .to_string()
    };

    confirm_action(req, res, env, session, client, signal_str);
}

inline void server::delete_user(const httplib::Request& req, httplib::Response& res, inja::Environment& env, ConfirmHandler& confirm_handler, Session& session, const Client& client)
{
    if (!is_logged_and_admin(req, res, session, client))
        return;

    const std::string user_id{ req.path_params.at("user_id") };
    const bool is_admin{ su::string_to_bool(req.get_param_value("is_admin")) };
    logging::debug{ "Delete {} {}", user_id, client.user_name(user_id) };

    const std::string suppressor_user_id{ connected_user_id(req, session) };

    const auto delete_action{ [user_id, is_admin, suppressor_user_id, &client] {
        client.delete_user(user_id);
        const std::string user_type{ is_admin ? "Admin" : "User" };
        logging::info{ "{} {} deleted by {}", user_type, user_id, suppressor_user_id };
    } };

    const auto redirect_action{ [is_admin](httplib::Response& res) {
        if (is_admin)
            res.set_redirect("/admin-list");
        else
            res.set_redirect("/user-list");
    } };

    const std::string signal_str{
        confirm_handler.create()
            ->on_confirm([delete_action, redirect_action](httplib::Response& res) {
                delete_action();
                redirect_action(res);
            })
            .on_deny([redirect_action](httplib::Response& res) {
                redirect_action(res);
            })
            .to_string()
    };

    confirm_action(req, res, env, session, client, signal_str);
}

namespace server
{
    static constexpr std::string_view changed_key{ "changed" };
    static constexpr std::string_view username_changed_value{ "username" };
    static constexpr std::string_view password_changed_value{ "password" };

    enum class AlertUpdateLoginUser : std::uint8_t
    {
        no_alert,
        invalid_username,
        invalid_password,
        username_changed
    };
    using namespace std::literals::string_view_literals;
    static constexpr std::array alert_update_login_user_texts{ ""sv, "Username already taken"sv, "Confirm password does not match"sv, "Username changed!"sv };

    enum class AlertUpdatePasswordUser : std::uint8_t
    {
        no_alert,
        invalid_old_password,
        invalid_new_password,
        password_not_match,
        password_changed
    };
    using namespace std::literals::string_view_literals;
    static constexpr std::array alert_update_password_user_texts{ ""sv, "Old password does not match"sv, "Invalid new password"sv, "New passwords do not match"sv, "Password changed!"sv };

    using AlertUpdateUser = std::variant<AlertUpdateLoginUser, AlertUpdatePasswordUser>;

    void set_update_user_content(httplib::Response& res, inja::Environment& env, const Client& client,
                                 const std::string& user_id, AlertUpdateUser alert);
}

inline void server::set_update_user_content(httplib::Response& res, inja::Environment& env, const Client& client,
                                            const std::string& user_id, AlertUpdateUser alert)
{
    const AlertUpdateLoginUser login_alert{
        std::holds_alternative<AlertUpdateLoginUser>(alert) ? std::get<AlertUpdateLoginUser>(alert) : AlertUpdateLoginUser::no_alert
    };
    const AlertUpdatePasswordUser password_alert{
        std::holds_alternative<AlertUpdatePasswordUser>(alert) ? std::get<AlertUpdatePasswordUser>(alert) : AlertUpdatePasswordUser::no_alert
    };
    const inja::json data{
        { "username", client.user_name(user_id) },
        { "login_alert", alert_to_text(login_alert, alert_update_login_user_texts) },
        { "password_alert", alert_to_text(password_alert, alert_update_password_user_texts) }
    };
    logging::debug{ data.dump() };
    const std::string body{ env.render(client.update_user_self_page(), data) }; // NOLINT(clang-analyzer-core.StackAddressEscape): in inja.hpp Parser::parse
    res.set_content(body, "text/html");
}

inline void server::update_user(const httplib::Request& req, httplib::Response& res, inja::Environment& env, Session& session, const Client& client)
{
    if (!is_logged(req, res, session, client))
        return;

    AlertUpdateUser alert;
    if (const std::string changed_value{ req.get_param_value(std::string{ changed_key }) };
        changed_value == username_changed_value)
        alert = AlertUpdateLoginUser::username_changed;
    else if (changed_value == password_changed_value)
        alert = AlertUpdatePasswordUser::password_changed;

    const std::string user_id{ connected_user_id(req, session) };
    set_update_user_content(res, env, client, user_id, alert);
}

inline void server::update_username(const httplib::Request& req, httplib::Response& res, inja::Environment& env, ConfirmHandler& /*confirm_handler*/, Session& session, const Client& client)
{
    if (!is_logged(req, res, session, client))
        return;

    const std::string user_id{ connected_user_id(req, session) };

    const std::string password{ crypto::sha512(req.get_param_value("password")) };
    const bool is_valid_user{ client.is_valid_user(user_id, password) };
    if (!is_valid_user) {
        set_update_user_content(res, env, client, user_id, AlertUpdateLoginUser::invalid_password);
        return;
    }

    std::string username{ req.get_param_value("username") };
    su::trim(username);
    su::lower(username);

    if (username_exists(username, client)) {
        set_update_user_content(res, env, client, user_id, AlertUpdateLoginUser::invalid_username);
        return;
    }

    client.update_username(user_id, username);
    res.set_redirect("/update-user" + std::format("?{}={}", changed_key, username_changed_value));
}

inline void server::update_password(const httplib::Request& req, httplib::Response& res, inja::Environment& env, ConfirmHandler& /*confirm_handler*/, Session& session, const Client& client)
{
    if (!is_logged(req, res, session, client))
        return;

    const std::string user_id{ connected_user_id(req, session) };

    const std::string old_password{ crypto::sha512(req.get_param_value("old-password")) };
    const bool is_valid_user{ client.is_valid_user(user_id, old_password) };
    if (!is_valid_user) {
        set_update_user_content(res, env, client, user_id, AlertUpdatePasswordUser::invalid_old_password);
        return;
    }

    const std::string new_password{ crypto::sha512(req.get_param_value("new-password")) };
    if (new_password.empty()) {
        set_update_user_content(res, env, client, user_id, AlertUpdatePasswordUser::invalid_new_password);
        return;
    }

    const std::string confirm_password{ crypto::sha512(req.get_param_value("confirm-password")) };
    if (new_password != confirm_password) {
        set_update_user_content(res, env, client, user_id, AlertUpdatePasswordUser::password_not_match);
        return;
    }

    client.update_password(user_id, new_password);
    res.set_redirect("/update-user" + std::format("?{}={}", changed_key, password_changed_value));
}

inline void server::group_list(const httplib::Request& req, httplib::Response& res, inja::Environment& env, const Session& session, const Client& client)
{
    if (!is_logged_and_admin(req, res, session, client))
        return;

    const std::vector<std::string> group_list{ client.group_list() };

    inja::json group_dict = inja::json::array();
    for (const std::string& group_id : group_list) {
        const std::vector group_users{ client.group_user_list(group_id) };
        std::vector<std::string> user_list(group_users.size());
        std::ranges::transform(group_users, user_list.begin(),
                               [&](const std::string& user_id) -> std::string {
                                   return client.user_name(user_id);
                               });
        const inja::json group = {
            { "id", group_id },
            { "name", client.group_name(group_id) },
            { "user_list", user_list }
        };
        group_dict += group;
    }

    const inja::json data{ { "group_dict", group_dict } };
    logging::debug{ data.dump() };

    const std::string body{ env.render(client.group_list_page(), data) }; // NOLINT(clang-analyzer-core.StackAddressEscape): in inja.hpp Parser::parse
    res.set_content(body, "text/html");
}

namespace server
{
    enum class AlertAddGroup : std::uint8_t
    {
        no_alert,
        invalid_group_name
    };
    using namespace std::literals::string_view_literals;
    static constexpr std::array alert_add_group_texts{ ""sv, "Group name already taken"sv };

    void set_add_group_content(httplib::Response& res, inja::Environment& env, const Client& client, AlertAddGroup alert);
}

inline void server::set_add_group_content(httplib::Response& res, inja::Environment& env, const Client& client, AlertAddGroup alert)
{
    const std::vector<std::string> user_list{ client.user_list() };

    inja::json user_dict = inja::json::array();
    for (const std::string& user_id : user_list) {
        const inja::json user{ { "id", user_id }, { "name", client.user_name(user_id) } };
        user_dict += user;
    }

    const inja::json data{
        { "user_dict", user_dict },
        { "alert", alert_to_text(alert, alert_add_group_texts) }
    };
    logging::debug{ data.dump() };

    const std::string body{ env.render(client.add_group_page(), data) }; // NOLINT(clang-analyzer-core.StackAddressEscape): in inja.hpp Parser::parse
    res.set_content(body, "text/html");
}

inline void server::add_group_get(const httplib::Request& req, httplib::Response& res, inja::Environment& env, const Session& session, const Client& client)
{
    if (!is_logged_and_admin(req, res, session, client))
        return;

    set_add_group_content(res, env, client, AlertAddGroup::no_alert);
}

inline void server::add_group_post(const httplib::Request& req, httplib::Response& res, inja::Environment& env, const Session& session, const Client& client)
{
    if (!is_logged_and_admin(req, res, session, client)) // NOLINT(clang-analyzer-core.StackAddressEscape): in inja.hpp Parser::parse
        return;

    std::string group_name{ req.get_param_value("name") };
    su::trim(group_name);
    su::lower(group_name);

    if (client.group_exists(group_name)) {
        set_add_group_content(res, env, client, AlertAddGroup::invalid_group_name);
        return;
    }

    const std::vector group_user_ids{ param_value_list(req, "user_ids") };
    const std::string group_id{ client.add_group(group_name, group_user_ids) };

    res.set_redirect("/group-list");

    const std::string creator_user_id{ connected_user_id(req, session) };
    logging::info{ "Group {} created by {}", group_id, creator_user_id };
}

namespace server
{
    enum class AlertUpdateGroup : std::uint8_t
    {
        no_alert,
        invalid_group_name
    };
    using namespace std::literals::string_view_literals;
    static constexpr std::array alert_update_group_texts{ ""sv, "Group name already taken"sv };

    void set_update_group_content(const httplib::Request& req, httplib::Response& res, inja::Environment& env, const Client& client, AlertUpdateGroup alert);
}

inline void server::set_update_group_content(const httplib::Request& req, httplib::Response& res, inja::Environment& env, const Client& client, AlertUpdateGroup alert)
{
    const std::string group_id{ req.path_params.at("group_id") };

    const std::string group_name{ client.group_name(group_id) };

    const std::vector<std::string> user_list{ client.user_list() };
    const std::vector<std::string> user_right_list{ client.group_user_list(group_id) };

    inja::json user_dict = inja::json::array();
    for (const std::string& user_id : user_list) {
        const bool selected{ std::ranges::find(user_right_list, user_id) != user_right_list.cend() };
        const inja::json user{ { "id", user_id }, { "name", client.user_name(user_id) }, { "selected", selected } };
        user_dict += user;
    }

    const inja::json data{
        { "group_id", group_id },
        { "group_name", group_name },
        { "user_dict", user_dict },
        { "alert", alert_to_text(alert, alert_update_group_texts) }
    };
    logging::debug{ data.dump() };

    const std::string body{ env.render(client.update_group_page(), data) }; // NOLINT(clang-analyzer-core.StackAddressEscape): in inja.hpp Parser::parse
    res.set_content(body, "text/html");
}

inline void server::update_group_get(const httplib::Request& req, httplib::Response& res, inja::Environment& env, Session& session, const Client& client)
{
    if (!is_logged_and_admin(req, res, session, client))
        return;

    set_update_group_content(req, res, env, client, AlertUpdateGroup::no_alert);
}

inline void server::update_group_post(const httplib::Request& req, httplib::Response& res, inja::Environment& env, ConfirmHandler& confirm_handler, Session& session, const Client& client)
{
    if (!is_logged_and_admin(req, res, session, client)) // NOLINT(clang-analyzer-core.StackAddressEscape): in inja.hpp Parser::parse
        return;

    const std::string group_id{ req.path_params.at("group_id") };

    std::string group_name{ req.get_param_value("name") };
    su::trim(group_name);
    su::lower(group_name);

    if (group_name != client.group_name(group_id) && client.group_exists(group_name)) {
        set_update_group_content(req, res, env, client, AlertUpdateGroup::invalid_group_name);
        return;
    }

    const std::vector group_user_ids{ param_value_list(req, "user_ids") };

    const std::string updater_user_id{ connected_user_id(req, session) };

    const std::string signal_str{
        confirm_handler.create()
            ->on_confirm([group_id, group_name, group_user_ids, updater_user_id, &client](httplib::Response& res) {
                client.update_group(group_id, group_name, group_user_ids);
                res.set_redirect("/group-list");
                logging::info{ "Group {} updated by {}", group_id, updater_user_id };
            })
            .on_deny([](httplib::Response& res) {
                res.set_redirect("/group-list");
            })
            .to_string()
    };

    confirm_action(req, res, env, session, client, signal_str);
}

inline void server::delete_group(const httplib::Request& req, httplib::Response& res, inja::Environment& env, ConfirmHandler& confirm_handler, Session& session, const Client& client)
{
    if (!is_logged_and_admin(req, res, session, client))
        return;

    const std::string group_id{ req.path_params.at("group_id") };
    logging::debug{ "Delete {} {}", group_id, client.group_name(group_id) };

    const std::string suppressor_user_id{ connected_user_id(req, session) };

    const std::string signal_str{
        confirm_handler.create()
            ->on_confirm([group_id, suppressor_user_id, &client](httplib::Response& res) {
                client.delete_group(group_id);
                res.set_redirect("/group-list");
                logging::info{ "Group {} deleted by {}", group_id, suppressor_user_id };
            })
            .on_deny([](httplib::Response& res) {
                res.set_redirect("/group-list");
            })
            .to_string()
    };

    confirm_action(req, res, env, session, client, signal_str);
}

inline void server::video_list(const httplib::Request& req, httplib::Response& res, inja::Environment& env, const Session& session, const Client& client)
{
    if (!is_logged_and_admin(req, res, session, client))
        return;

    const std::vector<std::string> video_list{ client.admin_video_list() };

    inja::json video_dict = server::video_dict(video_list, client);
    for (inja::json& video : video_dict) {
        {
            const std::vector rights{ client.video_group_right_list(video["id"]) };
            std::vector<std::string> group_rights(rights.size());
            std::ranges::transform(rights, group_rights.begin(),
                                   [&](const std::string& group_id) -> std::string {
                                       return client.group_name(group_id);
                                   });
            video += { "group_right_list", group_rights };
        }
        {
            const std::vector rights{ client.video_user_right_list(video["id"]) };
            std::vector<std::string> user_rights(rights.size());
            std::ranges::transform(rights, user_rights.begin(),
                                   [&](const std::string& user_id) -> std::string {
                                       return client.user_name(user_id);
                                   });
            video += { "user_right_list", user_rights };
        }
    }

    const inja::json data{ { "video_dict", video_dict } };
    logging::debug{ data.dump() };

    const std::string body{ env.render(client.video_list_page(), data) }; // NOLINT(clang-analyzer-core.StackAddressEscape): in inja.hpp Parser::parse
    res.set_content(body, "text/html");
}

namespace server
{
    constexpr const char* default_button_video_text_helper() { return "choose file"; }
    constexpr const char* default_video_text_helper() { return "or drag and drop file here"; }
    constexpr const char* default_video_title_placeholder() { return "Enter the video title here"; }
    void set_add_video_content(httplib::Response& res, inja::Environment& env, const Client& client,
                               const std::string& button_video_text_helper,
                               const std::string& video_text_helper, const std::string& video_title_placeholder);
}

inline void server::set_add_video_content(httplib::Response& res, inja::Environment& env, const Client& client,
                                          const std::string& button_video_text_helper,
                                          const std::string& video_text_helper, const std::string& video_title_placeholder)
{
    const std::vector group_list{ client.group_list() };

    inja::json group_dict = inja::json::array();
    for (const std::string& group_id : group_list) {
        const inja::json group{ { "id", group_id }, { "name", client.group_name(group_id) } };
        group_dict += group;
    }

    const std::vector user_list{ client.user_list() };

    inja::json user_dict = inja::json::array();
    for (const std::string& user_id : user_list) {
        const inja::json user{ { "id", user_id }, { "name", client.user_name(user_id) } };
        user_dict += user;
    }

    const inja::json data{
        { "group_dict", group_dict },
        { "user_dict", user_dict },
        { "button_video_text_helper", button_video_text_helper },
        { "video_text_helper", video_text_helper },
        { "video_title_placeholder", video_title_placeholder }
    };
    logging::debug{ data.dump() };

    const std::string body{ env.render(client.add_video_page(), data) }; // NOLINT(clang-analyzer-core.StackAddressEscape): in inja.hpp Parser::parse
    res.set_content(body, "text/html");
}

inline void server::add_video_get(const httplib::Request& req, httplib::Response& res, inja::Environment& env, const Session& session, const Client& client)
{
    if (!is_logged_and_admin(req, res, session, client))
        return;

    set_add_video_content(res, env, client, default_button_video_text_helper(), default_video_text_helper(), default_video_title_placeholder());
}

inline void server::add_video_post(const httplib::Request& req, httplib::Response& res, inja::Environment& env, const Session& session, const Client& client)
{
    if (!is_logged_and_admin(req, res, session, client))
        return;

    const std::string video_title{ req.form.get_field("title") };
    if (video_title.empty()) {
        set_add_video_content(res, env, client, default_button_video_text_helper(), default_video_text_helper(), "Enter a non-empty video title here");
        return;
    }

    if (!req.form.has_file("file")) {
        set_add_video_content(res, env, client, default_button_video_text_helper(), default_video_text_helper(), default_video_title_placeholder());
        return;
    }

    const httplib::FormData item{ req.form.get_file("file") };
    if (item.content_type != "video/mp4") {
        set_add_video_content(res, env, client, "choose mp4 file", default_video_text_helper(), default_video_title_placeholder());
        return;
    }

    const std::vector allowed_group_ids{ fields_list(req, "group_ids") };
    const std::vector allowed_user_ids{ fields_list(req, "user_ids") };
    const std::string video_id{ client.add_video(video_title, item.content, allowed_group_ids, allowed_user_ids) };
    res.set_redirect("/video-list");

    const std::string creator_user_id{ connected_user_id(req, session) };
    logging::info{ "Video {} added by {}", video_id, creator_user_id };
}

namespace server
{
    void set_update_video_content(const httplib::Request& req, httplib::Response& res, inja::Environment& env, const Client& client,
                                  const std::string& video_title_placeholder);
}

inline void server::set_update_video_content(const httplib::Request& req, httplib::Response& res, inja::Environment& env, const Client& client,
                                             const std::string& video_title_placeholder)
{
    const std::string video_id{ req.path_params.at("video_id") };

    const std::string video_title{ client.video_title(video_id) };

    const std::vector group_list{ client.group_list() };
    const std::vector group_right_list{ client.video_group_right_list(video_id) };

    inja::json group_dict = inja::json::array();
    for (const std::string& group_id : group_list) {
        const bool selected{ std::ranges::find(group_right_list, group_id) != group_right_list.cend() };
        const inja::json group{ { "id", group_id }, { "name", client.group_name(group_id) }, { "selected", selected } };
        group_dict += group;
    }

    const std::vector user_list{ client.user_list() };
    const std::vector user_right_list{ client.video_user_right_list(video_id) };

    inja::json user_dict = inja::json::array();
    for (const std::string& user_id : user_list) {
        const bool selected{ std::ranges::find(user_right_list, user_id) != user_right_list.cend() };
        const inja::json user{ { "id", user_id }, { "name", client.user_name(user_id) }, { "selected", selected } };
        user_dict += user;
    }

    const inja::json data{
        { "video_id", video_id },
        { "video_title", video_title },
        { "video_title_placeholder", video_title_placeholder },
        { "group_dict", group_dict },
        { "user_dict", user_dict }
    };
    logging::debug{ data.dump() };

    const std::string body{ env.render(client.update_video_page(), data) }; // NOLINT(clang-analyzer-core.StackAddressEscape): in inja.hpp Parser::parse
    res.set_content(body, "text/html");
}

inline void server::update_video_get(const httplib::Request& req, httplib::Response& res, inja::Environment& env, Session& session, const Client& client)
{
    if (!is_logged_and_admin(req, res, session, client))
        return;

    set_update_video_content(req, res, env, client, default_video_title_placeholder());
}

inline void server::update_video_post(const httplib::Request& req, httplib::Response& res, inja::Environment& env, ConfirmHandler& confirm_handler, Session& session, const Client& client)
{
    if (!is_logged_and_admin(req, res, session, client))
        return;

    const std::string video_id{ req.path_params.at("video_id") };

    const std::string video_title{ req.get_param_value("title") };
    if (video_title.empty()) {
        set_update_video_content(req, res, env, client, "Enter a non-empty video title here");
        return;
    }

    const std::vector allowed_group_ids{ param_value_list(req, "group_ids") };
    const std::vector allowed_user_ids{ param_value_list(req, "user_ids") };

    const std::string updater_user_id{ connected_user_id(req, session) };

    const std::string signal_str{
        confirm_handler.create()
            ->on_confirm([video_id, video_title, allowed_group_ids, allowed_user_ids, updater_user_id, &client](httplib::Response& res) {
                client.update_video(video_id, video_title, allowed_group_ids, allowed_user_ids);
                res.set_redirect("/video-list");
                logging::info{ "Video {} updated by {}", video_id, updater_user_id };
            })
            .on_deny([](httplib::Response& res) {
                res.set_redirect("/video-list");
            })
            .to_string()
    };

    confirm_action(req, res, env, session, client, signal_str);
}

inline void server::delete_video(const httplib::Request& req, httplib::Response& res, inja::Environment& env, ConfirmHandler& confirm_handler, Session& session, const Client& client)
{
    if (!is_logged_and_admin(req, res, session, client))
        return;

    const std::string video_id{ req.path_params.at("video_id") };
    logging::debug{ "Delete {} {}", video_id, client.video_title(video_id) };

    const std::string suppressor_user_id{ connected_user_id(req, session) };

    const std::string signal_str{
        confirm_handler.create()
            ->on_confirm([video_id, suppressor_user_id, &client](httplib::Response& res) {
                client.delete_video(video_id);
                res.set_redirect("/video-list");
                logging::info{ "Video {} deleted by {}", video_id, suppressor_user_id };
            })
            .on_deny([](httplib::Response& res) {
                res.set_redirect("/video-list");
            })
            .to_string()
    };

    confirm_action(req, res, env, session, client, signal_str);
}

inline void server::download_video(const httplib::Request& req, httplib::Response& res, const Session& session, const Client& client)
{
    if (!is_logged_and_admin(req, res, session, client))
        return;

    const std::string video_id{ req.path_params.at("video_id") };

    const std::size_t video_size{ static_cast<std::size_t>(client.video_size(video_id)) };

    const std::string downloader_user_id{ connected_user_id(req, session) };

    res.set_content_provider(
        video_size,  // Content length
        "video/mp4", // Content type
        [video_id, &client](std::size_t offset, std::size_t length, httplib::DataSink& sink) -> bool {
            const std::string chunk{ client.video(video_id, offset, std::min(length, video_chunk_size())) };
            sink.write(chunk.data(), chunk.size());
            return true; // return 'false' if you want to cancel the process.
        },
        [video_id, downloader_user_id](bool success) {
            logging::info{ "Video {} downloaded by {}: {}", video_id, downloader_user_id, success };
        });
}

namespace server
{
    bool has_video_right(const httplib::Request& req, const Session& session, const Client& client);
    constexpr std::string_view url_scheme();
    std::string meta_video_title(const std::string& video_title);
    std::string meta_video_description(const std::string& video_title);
    std::string meta_video_url(const httplib::Request& req, const std::string& video_id);
    std::string meta_video_image(const httplib::Request& req, const std::string& video_id);
}

inline bool server::has_video_right(const httplib::Request& req, const Session& session, const Client& client)
{
    const std::string cookie{ req.get_header_value("Cookie") };
    const std::string session_id{ Session::extract_session_id_from_cookie(cookie) };
    const bool is_logged{ session.is_valid_session(session_id) };

    const std::string video_id{ req.path_params.at("video_id") };

    if (is_logged) {
        const std::string& connected_user_id{ session.user_from_session(session_id) };
        if (!client.has_video_right(video_id, connected_user_id)) {
            return false;
        }
    } else if (!client.has_video_right(video_id)) {
        return false;
    }

    return true;
}

constexpr std::string_view server::url_scheme()
{
#ifdef _DEBUG
    return "http";
#else
    return "https";
#endif
}

inline std::string server::meta_video_title(const std::string& video_title)
{
    return video_title;
}

inline std::string server::meta_video_description(const std::string& video_title)
{
    return "Watch " + video_title + " video";
}

inline std::string server::meta_video_url(const httplib::Request& req, const std::string& video_id)
{
    return std::string(url_scheme()) + "://" + req.get_header_value("Host") + "/watch-video/" + video_id;
}

inline std::string server::meta_video_image(const httplib::Request& req, const std::string& video_id)
{
    return std::string(url_scheme()) + "://" + req.get_header_value("Host") + "/thumbnail/" + video_id;
}

inline void server::watch_video(const httplib::Request& req, httplib::Response& res, inja::Environment& env, Session& session, const Client& client)
{
    const bool is_forbidden{ !has_video_right(req, session, client) };
    const std::string video_id{ req.path_params.at("video_id") };

    // for login redirect
    if (is_forbidden) {
        const std::string session_id{ session.create_not_logged_session() };
        session.insert_value_from_session(session_id, "video_id", video_id);
        res.set_header("Set-Cookie", session.insert_session_id_to_cookie(req.get_header_value("Host"), session_id));
    }

    const std::string cookie{ req.get_header_value("Cookie") };
    const bool is_logged{ session.is_valid_session_from_cookie(cookie) };

    const std::string video_title{ client.video_title(video_id) };
    const int video_views{ client.video_views(video_id) };

    const inja::json data{
        { "is_logged", is_logged },
        { "meta_video_title", meta_video_title(video_title) },
        { "meta_video_description", meta_video_description(video_title) },
        { "meta_video_url", meta_video_url(req, video_id) },
        { "meta_video_image", meta_video_image(req, video_id) },
        { "is_forbidden", is_forbidden },
        { "video_id", video_id },
        { "title", video_title },
        { "views", video_views }
    };
    logging::debug{ data.dump() };

    const std::string body{ env.render(client.watch_video_page(), data) }; // NOLINT(clang-analyzer-core.StackAddressEscape): in inja.hpp Parser::parse
    res.set_content(body, "text/html");
}

namespace server
{
    bool request_from_watch_video(const httplib::Request& req, httplib::Response& res, const std::string& video_id);
}

inline bool server::request_from_watch_video(const httplib::Request& req, httplib::Response& res, const std::string& video_id)
{
    const std::string referrer{ req.get_header_value("Referer") };
    if (referrer.ends_with("/watch-video/" + video_id) || referrer.ends_with("/static/js/videoserviceworker.js")) {
        return true;
    }

    res.status = httplib::StatusCode::Forbidden_403;
    return false;
}

inline void server::video_playlist(const httplib::Request& req, httplib::Response& res, const Session& session, const Client& client)
{
    if (!has_video_right(req, session, client)) {
        res.status = httplib::StatusCode::Forbidden_403;
        return;
    }

    const std::string video_id{ req.path_params.at("video_id") };

    // block video if not in watch-video page
    if (!request_from_watch_video(req, res, video_id))
        return;

    const std::string playlist_content{ client.video_playlist(video_id) };
    res.set_content(playlist_content, "application/vnd.apple.mpegurl");
}

inline void server::video_segment(const httplib::Request& req, httplib::Response& res, const Session& session, const Client& client)
{
    if (!has_video_right(req, session, client)) {
        res.status = httplib::StatusCode::Forbidden_403;
        return;
    }

    const std::string video_id{ req.path_params.at("video_id") };
    const std::string segment{ req.path_params.at("segment") };

    // block video if not in watch-video page
    if (!request_from_watch_video(req, res, video_id))
        return;

    const std::string segment_content{ client.video_segment(video_id, segment) };
    res.set_content(segment_content, "video/mp2t");
}

inline void server::increment_video_views(const httplib::Request& req, httplib::Response& res, const Session& session, const Client& client)
{
    if (!has_video_right(req, session, client)) {
        res.status = httplib::StatusCode::Forbidden_403;
        return;
    }

    const std::string video_id{ req.path_params.at("video_id") };

    // block increment if not in watch-video page
    if (!request_from_watch_video(req, res, video_id))
        return;

    client.increment_video_views(video_id);
}

inline void server::thumbnail(const httplib::Request& req, httplib::Response& res, const Session& /*session*/, const Client& client)
{
    // don't test to share link image
    // if (!has_video_right(req, res, session, client))
    //    return;

    const std::string video_id{ req.path_params.at("video_id") };
    const std::string thumbnail_content{ client.thumbnail(video_id) };
    res.set_content(thumbnail_content, "image/png");
}
