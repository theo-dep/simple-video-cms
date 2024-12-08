#include "server.h"

#include "chunkworker.h"
#include "client.h"
#include "confirmhandler.h"
#include "crypto.h"
#include "logging.h"
#include "servercommon.h"
#include "session.h"
#include "stringutils.h"

#include <httplib.h>
#include <inja.hpp>

namespace server
{
    void set_no_cache_headers(httplib::Response& res) noexcept;

    void set_error_handler(httplib::Server& server, const Client& client) noexcept;
    void set_exception_handler(httplib::Server& server) noexcept;
    void set_logger(httplib::Server& server) noexcept;

    bool is_logged_and_admin(const httplib::Request& req, httplib::Response& res, const Session& session, const Client& client) noexcept;

    // inja exceptions catched by httplib Server::set_exception_handler

    inja::json video_dict(const std::vector<std::string>& video_ids, const Client& client);

    void static_file(const httplib::Request& req, httplib::Response& res, const Client& client) noexcept;

    void home(const httplib::Request& req, httplib::Response& res, inja::Environment& env, const Session& session, const Client& client);
    void dashboard(const httplib::Request& req, httplib::Response& res, inja::Environment& env, const Session& session, const Client& client);

    void login_get(const httplib::Request& req, httplib::Response& res, inja::Environment& env, Session& session, const Client& client);
    void login_post(const httplib::Request& req, httplib::Response& res, inja::Environment& env, Session& session, const Client& client);
    void logout(const httplib::Request& req, httplib::Response& res, Session& session) noexcept;

    void admin_list(const httplib::Request& req, httplib::Response& res, inja::Environment& env, const Session& session, const Client& client);
    void user_list(const httplib::Request& req, httplib::Response& res, inja::Environment& env, const Session& session, const Client& client);

    void add_user_get(const httplib::Request& req, httplib::Response& res, inja::Environment& env, const Session& session, const Client& client);
    void add_user_post(const httplib::Request& req, httplib::Response& res, inja::Environment& env, const Session& session, const Client& client);

    void confirm_action(const httplib::Request& req, httplib::Response& res, Session& session, const Client& client, const std::string& confirm_signal_str) noexcept;
    void confirm(const httplib::Request& req, httplib::Response& res, ConfirmHandler& confirm_handler, Session& session, const Client& client) noexcept;

    void update_user_get(const httplib::Request& req, httplib::Response& res, inja::Environment& env, Session& session, const Client& client);
    void update_user_post(const httplib::Request& req, httplib::Response& res, inja::Environment& env, ConfirmHandler& confirm_handler, Session& session, const Client& client);
    void delete_user(const httplib::Request& req, httplib::Response& res, ConfirmHandler& confirm_handler, Session& session, const Client& client) noexcept;

    void video_list(const httplib::Request& req, httplib::Response& res, inja::Environment& env, const Session& session, const Client& client);
    void add_video_get(const httplib::Request& req, httplib::Response& res, inja::Environment& env, const Session& session, const Client& client);
    void add_video_post(const httplib::Request& req, httplib::Response& res, inja::Environment& env, const Session& session, const Client& client);
    void update_video_get(const httplib::Request& req, httplib::Response& res, inja::Environment& env, Session& session, const Client& client);
    void update_video_post(const httplib::Request& req, httplib::Response& res, ConfirmHandler& confirm_handler, Session& session, const Client& client) noexcept;
    void delete_video(const httplib::Request& req, httplib::Response& res, ConfirmHandler& confirm_handler, Session& session, const Client& client) noexcept;

    void watch_video(const httplib::Request& req, httplib::Response& res, inja::Environment& env, const Session& session, const Client& client);
    void video(const httplib::Request& req, httplib::Response& res, const Session& session, const Client& client) noexcept;
    void thumbnail(const httplib::Request& req, httplib::Response& res, const Session& session, const Client& client) noexcept;
}

int server::start() noexcept
{
    bool is_client_ok{ false };
    const Client client(is_client_ok);
    if (!is_client_ok) {
        logging::error{ "Fail to create the client" };
        return EXIT_FAILURE;
    }

    inja::Environment env;
    Session session;
    ConfirmHandler confirm_handler;
    httplib::Server server;

    set_error_handler(server, client);
    set_exception_handler(server);
    set_logger(server);

    server.set_post_routing_handler([](const httplib::Request& req, httplib::Response& res) noexcept {
        if (!req.path.contains("static")) {
            set_no_cache_headers(res);
        }
    });

    server
        .Get(sc::static_regexp_path(), sc::serve(static_file, std::cref(client)))

        .Get("/", sc::serve(home, std::ref(env), std::cref(session), std::cref(client)))
        .Get("/dashboard", sc::serve(dashboard, std::ref(env), std::cref(session), std::cref(client)))

        .Get("/login", sc::serve(login_get, std::ref(env), std::ref(session), std::cref(client)))
        .Post("/login", sc::serve(login_post, std::ref(env), std::ref(session), std::cref(client)))

        .Get("/logout", sc::serve(logout, std::ref(session)))

        .Get("/admin-list", sc::serve(admin_list, std::ref(env), std::cref(session), std::cref(client)))
        .Get("/user-list", sc::serve(user_list, std::ref(env), std::cref(session), std::cref(client)))

        .Get("/add-user", sc::serve(add_user_get, std::ref(env), std::cref(session), std::cref(client)))
        .Post("/add-user", sc::serve(add_user_post, std::ref(env), std::cref(session), std::cref(client)))

        .Post("/confirm", sc::serve(confirm, std::ref(confirm_handler), std::ref(session), std::cref(client)))

        .Get("/update-user/:user_id", sc::serve(update_user_get, std::ref(env), std::ref(session), std::cref(client)))
        .Post("/update-user/:user_id", sc::serve(update_user_post, std::ref(env), std::ref(confirm_handler), std::ref(session), std::cref(client)))
        .Get("/delete-user/:user_id", sc::serve(delete_user, std::ref(confirm_handler), std::ref(session), std::cref(client)))

        .Get("/video-list", sc::serve(video_list, std::ref(env), std::cref(session), std::cref(client)))

        .Get("/add-video", sc::serve(add_video_get, std::ref(env), std::cref(session), std::cref(client)))
        .Post("/add-video", sc::serve(add_video_post, std::ref(env), std::cref(session), std::cref(client)))
        .Get("/update-video/:video_id", sc::serve(update_video_get, std::ref(env), std::ref(session), std::cref(client)))
        .Post("/update-video/:video_id", sc::serve(update_video_post, std::ref(confirm_handler), std::ref(session), std::cref(client)))
        .Get("/delete-video/:video_id", sc::serve(delete_video, std::ref(confirm_handler), std::ref(session), std::cref(client)))

        .Get("/watch-video/:video_id", sc::serve(watch_video, std::ref(env), std::cref(session), std::cref(client)))
        .Get("/video/:video_id", sc::serve(video, std::cref(session), std::cref(client)))
        .Get("/thumbnail/:video_id", sc::serve(thumbnail, std::cref(session), std::cref(client)));

    constexpr const char* host{ "0.0.0.0" };
    constexpr int port{ 8080 };
    logging::info{ "Serving HTTP on {0} port {1} ...", host, port };
    return (server.listen(host, port) ? EXIT_SUCCESS : EXIT_FAILURE);
}

inline void server::set_no_cache_headers(httplib::Response& res) noexcept
{
    res.set_header("Last-Modified", logging::time_local());
    res.set_header("Cache-Control", "no-store, no-cache, must-revalidate, post-check=0, pre-check=0, max-age=0");
    res.set_header("Pragma", "no-cache");
    res.set_header("Expires", "-1");
}

inline void server::set_error_handler(httplib::Server& server, const Client& client) noexcept
{
    server.set_error_handler([&client](const httplib::Request& /*req*/, httplib::Response& res) noexcept {
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

        // body = env.render(body, _data);
        res.set_content(body, "text/html");
    });
}

inline void server::set_exception_handler(httplib::Server& server) noexcept
{
    server.set_exception_handler([](const httplib::Request& /*req*/, httplib::Response& res, std::exception_ptr ep) noexcept {
        constexpr int error_code{ httplib::StatusCode::InternalServerError_500 };
        std::string message;
        try {
            std::rethrow_exception(std::move(ep));
        } catch (const std::exception& e) {
            message = e.what();
        } catch (...) {
            message = "Unknown Exception";
        }

        logging::error{ message };

        std::string body{ Client::generic_error(error_code, message) };
        // body = env.render(body, _data);

        res.set_content(body, "text/html");
        res.status = error_code;
    });
}

inline void server::set_logger(httplib::Server& server) noexcept
{
    server.set_logger([](const httplib::Request& req, const httplib::Response& res) noexcept {
        logging::raw_log(sc::log(req, res));
    });
}

inline bool server::is_logged_and_admin(const httplib::Request& req, httplib::Response& res, const Session& session, const Client& client) noexcept
{
    const std::string cookie{ req.get_header_value("Cookie") };
    const std::string session_id{ Session::extract_session_id_from_cookie(cookie) };
    if (!session.is_valid_session(session_id)) {
        res.set_redirect("/login");
        return false;
    }

    if (!client.is_admin(session.user_from_session(session_id))) {
        const std::string body{ client.error_page_403() };
        res.set_content(body, "text/html");
        return false;
    }

    return true;
}

inline inja::json server::video_dict(const std::vector<std::string>& video_ids, const Client& client)
{
    inja::json video_dict;
    for (const std::string& video_id : video_ids) {
        const std::string title{ client.video_title(video_id) };
        const std::int64_t views{ client.video_views(video_id) };
        video_dict.emplace_back(inja::json::object({ { "id", video_id }, { "title", title }, { "views", views } }));
    }
    return video_dict;
}

inline void server::static_file(const httplib::Request& req, httplib::Response& res, const Client& client) noexcept
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
    const std::string& connected_user{ session.user_from_session(session_id) };
    if (is_logged && client.is_admin(connected_user)) {
        res.set_redirect("/dashboard");
        return;
    }

    std::vector<std::string> video_ids;
    std::vector<std::string> logged_video_ids;
    if (req.has_param("search")) {
        const std::string search{ req.get_param_value("search") };
        video_ids = client.no_right_video_list(search);
        if (is_logged) {
            logged_video_ids = client.video_list(connected_user, search);
        }
    } else {
        video_ids = client.no_right_video_list();
        if (is_logged) {
            logged_video_ids = client.video_list(connected_user);
        }
    }

#ifdef __cpp_lib_containers_ranges
    video_ids.append_range(logged_video_ids);
#else
    video_ids.insert(video_ids.end(), logged_video_ids.cbegin(), logged_video_ids.cend());
#endif

    const inja::json data{
        { "is_logged", is_logged },
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
        { "video_count", client.video_count() },
        { "view_count", client.view_count() }
    };
    logging::debug{ data.dump() };

    const std::string body{ env.render(client.dashboard_page(), data) }; // NOLINT(clang-analyzer-core.StackAddressEscape): in inja.hpp Parser::parse
    res.set_content(body, "text/html");
}

namespace server
{
    void set_login_content(httplib::Response& res, inja::Environment& env, const Client& client, bool login_error);
}

inline void server::set_login_content(httplib::Response& res, inja::Environment& env, const Client& client, bool login_error)
{
    const inja::json data{ { "login_error", login_error } };
    logging::debug{ data.dump() };
    const std::string body{ env.render(client.login_page(), data) }; // NOLINT(clang-analyzer-core.StackAddressEscape): in inja.hpp Parser::parse
    res.set_content(body, "text/html");
}

inline void server::login_get(const httplib::Request& req, httplib::Response& res, inja::Environment& env, Session& session, const Client& client)
{
    const std::string cookie{ req.get_header_value("Cookie") };
    if (session.is_valid_session_from_cookie(cookie)) {
        res.set_redirect("/");
        return;
    }

    set_login_content(res, env, client, false);
}

inline void server::login_post(const httplib::Request& req, httplib::Response& res, inja::Environment& env, Session& session, const Client& client)
{
    const std::string password{ crypto::sha512(req.get_param_value("password")) };
    if (password.empty()) {
        set_login_content(res, env, client, true);
        return;
    }

    std::string username{ req.get_param_value("username") };
    su::trim(username);
    su::lower(username);

    const std::string user_id{ client.user_id(username) };
    const bool is_valid_user{ client.is_valid_user(user_id, password) };
    if (is_valid_user) {
        const std::string session_id{ session.create_session(user_id) };
        res.set_header("Set-Cookie", Session::insert_session_id_to_cookie(session_id));
        res.set_redirect("/");
    } else {
        set_login_content(res, env, client, true);
    }
}

inline void server::logout(const httplib::Request& req, httplib::Response& res, Session& session) noexcept
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
            { "is_super_admin", is_super_admin }
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
        const inja::json user = {
            { "id", user_id },
            { "name", client.user_name(user_id) }
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
    void set_add_user_content(httplib::Response& res, inja::Environment& env, const Client& client, bool is_admin, bool create_error, bool invalid_username);
}

inline void server::set_add_user_content(httplib::Response& res, inja::Environment& env, const Client& client, bool is_admin, bool create_error, bool invalid_username)
{
    const inja::json data{
        { "is_admin", is_admin },
        { "create_error", create_error },
        { "invalid_username", invalid_username }
    };
    logging::debug{ data.dump() };
    const std::string body{ env.render(client.add_user_page(), data) }; // NOLINT(clang-analyzer-core.StackAddressEscape): in inja.hpp Parser::parse
    res.set_content(body, "text/html");
}

inline void server::add_user_get(const httplib::Request& req, httplib::Response& res, inja::Environment& env, const Session& session, const Client& client)
{
    if (!is_logged_and_admin(req, res, session, client)) // NOLINT(clang-analyzer-core.StackAddressEscape): in inja.hpp Parser::parse
        return;

    const bool is_admin{ su::string_to_bool(req.get_param_value("is_admin")) };
    set_add_user_content(res, env, client, is_admin, false, false);
}

inline void server::add_user_post(const httplib::Request& req, httplib::Response& res, inja::Environment& env, const Session& session, const Client& client)
{
    if (!is_logged_and_admin(req, res, session, client)) // NOLINT(clang-analyzer-core.StackAddressEscape): in inja.hpp Parser::parse
        return;

    const bool is_admin_req{ su::string_to_bool(req.get_param_value("is_admin")) };
    const std::string password{ crypto::sha512(req.get_param_value("password")) };
    if (password.empty()) {
        set_add_user_content(res, env, client, is_admin_req, true, false);
        return;
    }

    const std::string confirm_password{ crypto::sha512(req.get_param_value("confirm-password")) };
    if (confirm_password != password) {
        set_add_user_content(res, env, client, is_admin_req, true, false);
        return;
    }

    std::string username{ req.get_param_value("username") };
    su::trim(username);
    su::lower(username);

    const std::string user_id{ client.user_id(username) };
    const bool is_user{ client.is_user(user_id) };
    const bool is_admin{ client.is_admin(user_id) };
    if ((is_user && !is_admin_req) || (is_admin && is_admin_req)) {
        set_add_user_content(res, env, client, is_admin_req, false, true);
    } else if (is_admin_req) {
        client.add_admin(username, password);
        res.set_redirect("/admin-list");
    } else {
        client.add_user(username, password);
        res.set_redirect("/user-list");
    }
}

namespace server
{
    constexpr const char* session_confirm_key() { return "Confirm-Signal"; }
}

inline void server::confirm_action(const httplib::Request& req, httplib::Response& res, Session& session, const Client& client, const std::string& confirm_signal_str) noexcept
{
    if (!is_logged_and_admin(req, res, session, client))
        return;

    const std::string cookie{ req.get_header_value("Cookie") };
    const std::string session_id{ Session::extract_session_id_from_cookie(cookie) };
    session.insert_value_from_session(session_id, session_confirm_key(), confirm_signal_str);

    const std::string body{ client.confirm_action_page() };
    res.set_content(body, "text/html");
}

inline void server::confirm(const httplib::Request& req, httplib::Response& res, ConfirmHandler& confirm_handler, Session& session, const Client& client) noexcept
{
    if (!is_logged_and_admin(req, res, session, client))
        return;

    const std::string cookie{ req.get_header_value("Cookie") };
    const std::string session_id{ Session::extract_session_id_from_cookie(cookie) };
    const std::string& confirm_signal_str{ session(session_id, session_confirm_key()) };

    const bool confirm{ su::string_to_bool(req.get_param_value("response")) };
    confirm_handler.confirm(res, confirm_signal_str, confirm);

    session.remove_value_from_session(session_id, session_confirm_key());
}

namespace server
{
    void set_update_user_content(httplib::Response& res, inja::Environment& env, const Client& client, const std::string& user_id, bool is_admin, bool login_error, bool update_error);
}

inline void server::set_update_user_content(httplib::Response& res, inja::Environment& env, const Client& client, const std::string& user_id, bool is_admin, bool login_error, bool update_error)
{
    const inja::json data{
        { "user", { { "id", user_id }, { "name", client.user_name(user_id) } } },
        { "is_admin", is_admin },
        { "login_error", login_error },
        { "update_error", update_error },
    };
    logging::debug{ data.dump() };
    const std::string body{ env.render(client.update_user_page(), data) }; // NOLINT(clang-analyzer-core.StackAddressEscape): in inja.hpp Parser::parse
    res.set_content(body, "text/html");
}

inline void server::update_user_get(const httplib::Request& req, httplib::Response& res, inja::Environment& env, Session& session, const Client& client)
{
    if (!is_logged_and_admin(req, res, session, client)) // NOLINT(clang-analyzer-core.StackAddressEscape): in inja.hpp Parser::parse
        return;

    const std::string user_id{ req.path_params.at("user_id") };
    const bool is_admin{ su::string_to_bool(req.get_param_value("is_admin")) };
    set_update_user_content(res, env, client, user_id, is_admin, false, false);
}

inline void server::update_user_post(const httplib::Request& req, httplib::Response& res, inja::Environment& env, ConfirmHandler& confirm_handler, Session& session, const Client& client)
{
    if (!is_logged_and_admin(req, res, session, client)) // NOLINT(clang-analyzer-core.StackAddressEscape): in inja.hpp Parser::parse
        return;

    const std::string user_id{ req.path_params.at("user_id") };
    const bool is_admin{ su::string_to_bool(req.get_param_value("is_admin")) };

    const std::string old_password{ crypto::sha512(req.get_param_value("old-password")) };
    const bool is_valid_user{ client.is_valid_user(user_id, old_password) };
    if (!is_valid_user) {
        set_update_user_content(res, env, client, user_id, is_admin, true, false);
        return;
    }

    const std::string new_password{ crypto::sha512(req.get_param_value("new-password")) };
    if (new_password.empty()) {
        set_update_user_content(res, env, client, user_id, is_admin, false, true);
        return;
    }

    const std::string confirm_password{ crypto::sha512(req.get_param_value("confirm-password")) };
    if (new_password != confirm_password) {
        set_update_user_content(res, env, client, user_id, is_admin, false, true);
        return;
    }

    std::string signal_str;
    if (is_admin) {
        signal_str = confirm_handler.create()
                         .on_confirm([user_id, new_password, &client](httplib::Response& res) noexcept {
                             client.update_user(user_id, new_password);
                             res.set_redirect("/admin-list");
                         })
                         .on_deny([](httplib::Response& res) noexcept {
                             res.set_redirect("/admin-list");
                         })
                         .to_string();
    } else {
        signal_str = confirm_handler.create()
                         .on_confirm([user_id, new_password, &client](httplib::Response& res) noexcept {
                             client.update_user(user_id, new_password);
                             res.set_redirect("/user-list");
                         })
                         .on_deny([](httplib::Response& res) noexcept {
                             res.set_redirect("/user-list");
                         })
                         .to_string();
    }

    confirm_action(req, res, session, client, signal_str);
}

inline void server::delete_user(const httplib::Request& req, httplib::Response& res, ConfirmHandler& confirm_handler, Session& session, const Client& client) noexcept
{
    if (!is_logged_and_admin(req, res, session, client))
        return;

    const std::string user_id{ req.path_params.at("user_id") };
    const bool admin{ su::string_to_bool(req.get_param_value("is_admin")) };
    logging::debug{ "Delete {} {}", user_id, client.user_name(user_id) };

    std::string signal_str;
    if (admin) {
        signal_str = confirm_handler.create()
                         .on_confirm([user_id, &client](httplib::Response& res) noexcept {
                             client.delete_user(user_id);
                             res.set_redirect("/admin-list");
                         })
                         .on_deny([](httplib::Response& res) noexcept {
                             res.set_redirect("/admin-list");
                         })
                         .to_string();
    } else {
        signal_str = confirm_handler.create()
                         .on_confirm([user_id, &client](httplib::Response& res) noexcept {
                             client.delete_user(user_id);
                             res.set_redirect("/user-list");
                         })
                         .on_deny([](httplib::Response& res) noexcept {
                             res.set_redirect("/user-list");
                         })
                         .to_string();
    }

    confirm_action(req, res, session, client, signal_str);
}

inline void server::video_list(const httplib::Request& req, httplib::Response& res, inja::Environment& env, const Session& session, const Client& client)
{
    if (!is_logged_and_admin(req, res, session, client))
        return;

    const std::vector<std::string> video_list{ client.video_list() };

    inja::json video_dict = server::video_dict(video_list, client);
    for (inja::json& video : video_dict) {
        const std::vector rights{ client.video_right_list(video["id"]) };
        std::vector<std::string> user_rights(rights.size());
        std::ranges::transform(rights, user_rights.begin(),
                               [&](const std::string& user_id) noexcept -> std::string {
                                   return client.user_name(user_id);
                               });
        video += { "right_list", user_rights };
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
    const std::vector<std::string> user_list{ client.user_list() };

    inja::json user_dict = inja::json::array();
    for (const std::string& user_id : user_list) {
        const inja::json user{ { "id", user_id }, { "name", client.user_name(user_id) } };
        user_dict += user;
    }

    const inja::json data{
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

    const std::string video_title{ req.get_file_value("title").content };
    if (video_title.empty()) {
        set_add_video_content(res, env, client, default_button_video_text_helper(), default_video_text_helper(), "Enter a non-empty video title here");
        return;
    }

    if (!req.has_file("file")) {
        set_add_video_content(res, env, client, default_button_video_text_helper(), default_video_text_helper(), default_video_title_placeholder());
        return;
    }

    const httplib::MultipartFormData item{ req.get_file_value("file") };
    if (item.content_type != "video/mp4") {
        set_add_video_content(res, env, client, "choose mp4 file", default_video_text_helper(), default_video_title_placeholder());
        return;
    }

    const std::vector user_id_items{ req.get_file_values("user_ids") };
    std::vector<std::string> allowed_user_ids(user_id_items.size());
    std::ranges::transform(user_id_items, allowed_user_ids.begin(),
                           [](const httplib::MultipartFormData& item) noexcept -> std::string { return item.content; });

    client.add_video(video_title, item.content, allowed_user_ids);
    res.set_redirect("/video-list");
}

inline void server::update_video_get(const httplib::Request& req, httplib::Response& res, inja::Environment& env, Session& session, const Client& client)
{
    if (!is_logged_and_admin(req, res, session, client))
        return;

    const std::string video_id{ req.path_params.at("video_id") };

    const std::vector<std::string> user_list{ client.user_list() };
    const std::vector<std::string> user_right_list{ client.video_right_list(video_id) };

    inja::json user_dict = inja::json::array();
    for (const std::string& user_id : user_list) {
        const bool checked{ std::ranges::find(user_right_list, user_id) != user_right_list.cend() };
        const inja::json user{ { "id", user_id }, { "name", client.user_name(user_id) }, { "checked", checked } };
        user_dict += user;
    }

    const inja::json data{
        { "video_id", video_id },
        { "user_dict", user_dict }
    };
    logging::debug{ data.dump() };

    const std::string body{ env.render(client.update_video_page(), data) }; // NOLINT(clang-analyzer-core.StackAddressEscape): in inja.hpp Parser::parse
    res.set_content(body, "text/html");
}

inline void server::update_video_post(const httplib::Request& req, httplib::Response& res, ConfirmHandler& confirm_handler, Session& session, const Client& client) noexcept
{
    if (!is_logged_and_admin(req, res, session, client))
        return;

    const std::string video_id{ req.path_params.at("video_id") };

    const std::size_t user_id_count{ req.get_param_value_count("user_ids") };
    std::vector<std::string> allowed_user_ids(user_id_count);
    for (std::size_t i{ 0 }; i < user_id_count; ++i) {
        allowed_user_ids[i] = req.get_param_value("user_ids", i);
    }

    const std::string signal_str{
        confirm_handler.create()
            .on_confirm([video_id, allowed_user_ids, &client](httplib::Response& res) noexcept {
                client.update_video(video_id, allowed_user_ids);
                res.set_redirect("/video-list");
            })
            .on_deny([](httplib::Response& res) noexcept {
                res.set_redirect("/video-list");
            })
            .to_string()
    };

    confirm_action(req, res, session, client, signal_str);
}

inline void server::delete_video(const httplib::Request& req, httplib::Response& res, ConfirmHandler& confirm_handler, Session& session, const Client& client) noexcept
{
    if (!is_logged_and_admin(req, res, session, client))
        return;

    const std::string video_id{ req.path_params.at("video_id") };
    logging::debug{ "Delete {} {}", video_id, client.video_title(video_id) };

    const std::string signal_str{
        confirm_handler.create()
            .on_confirm([video_id, &client](httplib::Response& res) noexcept {
                client.delete_video(video_id);
                res.set_redirect("/video-list");
            })
            .on_deny([](httplib::Response& res) noexcept {
                res.set_redirect("/video-list");
            })
            .to_string()
    };

    confirm_action(req, res, session, client, signal_str);
}

namespace server
{
    bool has_video_right(const httplib::Request& req, httplib::Response& res, const Session& session, const Client& client) noexcept;
}

inline bool server::has_video_right(const httplib::Request& req, httplib::Response& res, const Session& session, const Client& client) noexcept
{
    const std::string cookie{ req.get_header_value("Cookie") };
    const std::string session_id{ Session::extract_session_id_from_cookie(cookie) };
    const bool is_logged{ session.is_valid_session(session_id) };

    const std::string video_id{ req.path_params.at("video_id") };

    if (is_logged) {
        const std::string& connected_user_id{ session.user_from_session(session_id) };
        if (!client.has_video_right(video_id, connected_user_id)) {
            const std::string body{ client.error_page_403() };
            res.set_content(body, "text/html");
            return false;
        }
    } else if (!client.has_video_right(video_id)) {
        res.set_redirect("/login");
        return false;
    }

    return true;
}

inline void server::watch_video(const httplib::Request& req, httplib::Response& res, inja::Environment& env, const Session& session, const Client& client)
{
    if (!has_video_right(req, res, session, client)) // NOLINT(clang-analyzer-core.StackAddressEscape): in inja.hpp Parser::parse
        return;

    const std::string video_id{ req.path_params.at("video_id") };
    client.increment_video_views(video_id);

    const std::string cookie{ req.get_header_value("Cookie") };
    const bool is_logged{ session.is_valid_session_from_cookie(cookie) };

    const std::string video_title{ client.video_title(video_id) };
    const std::int64_t video_views{ client.video_views(video_id) };

    const inja::json data{
        { "is_logged", is_logged },
        { "video_id", video_id },
        { "title", video_title },
        { "views", video_views }
    };
    logging::debug{ data.dump() };

    const std::string body{ env.render(client.watch_video_page(), data) }; // NOLINT(clang-analyzer-core.StackAddressEscape): in inja.hpp Parser::parse
    res.set_content(body, "text/html");
}

inline void server::video(const httplib::Request& req, httplib::Response& res, const Session& session, const Client& client) noexcept
{
    if (!has_video_right(req, res, session, client))
        return;

    const std::string video_id{ req.path_params.at("video_id") };

    // block video if not in watch-video page
    const std::string& referer{ req.get_header_value("Referer") };
    if (!referer.ends_with("/watch-video/" + video_id)) {
        const std::string body{ client.error_page_403() };
        res.set_content(body, "text/html");
        return;
    }

    const std::size_t video_size{ static_cast<std::size_t>(client.video_size(video_id)) };

    ChunkWorker* const chunk_worker{ new ChunkWorker() };
    chunk_worker->set_buffer_size(video_size);
    chunk_worker->set_fetch_async_callback([chunk_worker, video_id, &req, &client]() noexcept -> bool {
        return client.video(video_id, req.get_header_value("Range"), chunk_worker->append_chunk_callback(true));
    });

    chunk_worker->start_chunk_at(req.get_header_value("Range"));
    chunk_worker->start_fetch_async();

    chunk_worker->wait_for_chunk();

    res.set_content_provider(
        video_size,  // Content length
        "video/mp4", // Content type
        [chunk_worker](std::size_t /*offset*/, std::size_t /*length*/, httplib::DataSink& sink) noexcept -> bool {
            const std::string chunk{ chunk_worker->chunk() };
            sink.write(&chunk[0], chunk.size());
            return chunk_worker->fetch_result(); // return 'false' will cancel the process.
        },
        [chunk_worker](bool) noexcept { delete chunk_worker; });
}

inline void server::thumbnail(const httplib::Request& req, httplib::Response& res, const Session& session, const Client& client) noexcept
{
    if (!has_video_right(req, res, session, client))
        return;

    const std::string video_id{ req.path_params.at("video_id") };
    const std::string thumbnail_content{ client.thumbnail(video_id) };
    res.set_content(thumbnail_content, "image/png");
}
