#include "server.h"

#include "apimodel.h"
#include "crypto.h"
#include "database.h"
#include "filesystem.h"
#include "logcontroller.h"
#include "logging.h"
#include "search.h"
#include "servercommon.h"
#include "session.h"
#include "stringutils.h"
#include "video.h"
#include "videosession.h"

#include <httplib.h>
#include <nlohmann/json.hpp>

#include <stacktrace>

namespace server
{
    bool create_super_admin(const Database& db);

    void set_logger(httplib::Server& server, LogController& video_log_controller);
    void set_exception_handler(httplib::Server& server);

    // Routes
    void watch_video(const httplib::Request& req, httplib::Response& res, const std::filesystem::path& bundle_dir, const Database& db);
    void index(const httplib::Request& req, httplib::Response& res, const std::filesystem::path& bundle_dir);

    // Auth
    void refresh(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);
    void login(const httplib::Request& req, httplib::Response& res, Session& session, const Database& db);
    void logout(const httplib::Request& req, httplib::Response& res, Session& session);
    void add_password(const httplib::Request& req, httplib::Response& res, const Database& db);
    void update_username(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);
    void update_password(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);

    // Video (user)
    void video_playlist(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db, VideoSession& video_session);
    void video_segment(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db, VideoSession& video_session);
    void thumbnail(const httplib::Request& req, httplib::Response& res, const Database& db);
    void start_video_session(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db, VideoSession& video_session);
    void reset_video_session(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db, VideoSession& video_session);

    // Admin - stats
    void admin_stats(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);

    // Admin - videos
    void admin_video_list(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);
    void admin_add_video(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);
    void admin_update_video(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);
    void admin_delete_video(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);
    void admin_download_video(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);

    // Admin - admins
    void admin_admin_list(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);
    void admin_add_admin(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);

    // Admin - users
    void admin_user_list(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);
    void admin_add_user(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);
    void admin_update_user(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);
    void admin_reset_user_password(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);
    void admin_delete_user(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);

    // Admin - groups
    void admin_group_list(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);
    void admin_add_group(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);
    void admin_update_group(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);
    void admin_delete_group(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);
}

int server::start()
{
    const std::filesystem::path database_file{ filesystem::data_path() / "video.db" };
    const Database db(database_file);
    if (!db.create_tables()) {
        logging::error{ "Fail to create database tables" };
        return EXIT_FAILURE;
    }

    if (!create_super_admin(db)) {
        logging::error{ "Fail to add super admin" };
        return EXIT_FAILURE;
    }

    httplib::Server server;
    Session session;
    VideoSession video_session;

#ifdef _DEBUG
    server.set_post_routing_handler([](const httplib::Request& /*req*/, httplib::Response& res) {
        res.set_header("Last-Modified", logging::time_local());
        res.set_header("Cache-Control", "no-store, no-cache, must-revalidate, post-check=0, pre-check=0, max-age=0");
        res.set_header("Pragma", "no-cache");
        res.set_header("Expires", "-1");
    });

    const std::filesystem::path source_dir{ std::filesystem::current_path() / "../../../" };
    server.set_mount_point("/build", (source_dir / "build/").string());
    server.set_mount_point("/node_modules", (source_dir / "node_modules/").string());
    const std::filesystem::path bundle_dir{ source_dir / "front/" };
#else
    const std::filesystem::path bundle_dir{ std::filesystem::current_path() };
#endif
    server.set_mount_point("/", bundle_dir.string());

    LogController video_log_controller("/video/");
    set_logger(server, video_log_controller);
    set_exception_handler(server);

    server
        .Get("/watch-video/:id", sc::serve(watch_video, std::cref(bundle_dir), std::cref(db)))
        .Get(R"((?!\/api\/).*)", sc::serve(index, std::cref(bundle_dir)))

        .Get("/api/refresh", sc::serve(refresh, std::cref(session), std::cref(db)))
        .Post("/api/login", sc::serve(login, std::ref(session), std::cref(db)))
        .Post("/api/logout", sc::serve(logout, std::ref(session)))
        .Post("/api/add-password", sc::serve(add_password, std::cref(db)))
        .Post("/api/update-username", sc::serve(update_username, std::cref(session), std::cref(db)))
        .Post("/api/update-password", sc::serve(update_password, std::cref(session), std::cref(db)))

        .Get("/api/thumbnail/:video_id", sc::serve(thumbnail, std::cref(db)))
        .Get("/api/video/:video_id/playlist", sc::serve(video_playlist, std::cref(session), std::cref(db), std::ref(video_session)))
        .Get("/api/video/:video_id/:segment", sc::serve(video_segment, std::cref(session), std::cref(db), std::ref(video_session)))
        .Post("/api/start-video-session/:video_id", sc::serve(start_video_session, std::cref(session), std::cref(db), std::ref(video_session)))
        .Post("/api/reset-video-session/:video_id", sc::serve(reset_video_session, std::cref(session), std::cref(db), std::ref(video_session)))

        .Get("/api/admin/stats", sc::serve(admin_stats, std::cref(session), std::cref(db)))

        .Get("/api/admin/video-list", sc::serve(admin_video_list, std::cref(session), std::cref(db)))
        .Post("/api/admin/add-video", sc::serve(admin_add_video, std::cref(session), std::cref(db)))
        .Post("/api/admin/update-video/:video_id", sc::serve(admin_update_video, std::cref(session), std::cref(db)))
        .Post("/api/admin/delete-video/:video_id", sc::serve(admin_delete_video, std::cref(session), std::cref(db)))
        .Get("/api/admin/download-video/:video_id", sc::serve(admin_download_video, std::cref(session), std::cref(db)))

        .Get("/api/admin/admin-list", sc::serve(admin_admin_list, std::cref(session), std::cref(db)))
        .Post("/api/admin/add-admin", sc::serve(admin_add_admin, std::cref(session), std::cref(db)))

        .Get("/api/admin/user-list", sc::serve(admin_user_list, std::cref(session), std::cref(db)))
        .Post("/api/admin/add-user", sc::serve(admin_add_user, std::cref(session), std::cref(db)))
        .Post("/api/admin/update-user/:user_id", sc::serve(admin_update_user, std::cref(session), std::cref(db)))
        .Post("/api/admin/reset-user-password/:user_id", sc::serve(admin_reset_user_password, std::cref(session), std::cref(db)))
        .Post("/api/admin/delete-user/:user_id", sc::serve(admin_delete_user, std::cref(session), std::cref(db)))

        .Get("/api/admin/group-list", sc::serve(admin_group_list, std::cref(session), std::cref(db)))
        .Post("/api/admin/add-group", sc::serve(admin_add_group, std::cref(session), std::cref(db)))
        .Post("/api/admin/update-group/:group_id", sc::serve(admin_update_group, std::cref(session), std::cref(db)))
        .Post("/api/admin/delete-group/:group_id", sc::serve(admin_delete_group, std::cref(session), std::cref(db)));

    { // runtime frontend environnement
        const std::string website_name{ sc::get_env("WEBSITE_NAME", "Simple Video CMS") };
        const std::string icon_path{ sc::get_env("ICON_PATH", {}) }; // bundled from front/assets
        const nlohmann::json env_json{
            { "websiteName", website_name },
            { "iconPath", icon_path }
        };
        const std::string env_content{ "window.__ENV__ = " + env_json.dump() + ";" };
        std::ofstream file(bundle_dir / "env.js", std::ios::out | std::ios::binary | std::ios::trunc);
        file.write(env_content.data(), static_cast<std::streamoff>(env_content.size()));
    }

    const std::string host{ sc::get_env("BACK_HOST", "0.0.0.0") };
    const int port{ su::string_to_int(sc::get_env("BACK_PORT", "8080")) };
    logging::info{ "Serving HTTP on http://{0}:{1} ...", host, port };
    return (server.listen(host, port) ? EXIT_SUCCESS : EXIT_FAILURE);
}

inline bool server::create_super_admin(const Database& db)
{
    const std::string username{ sc::get_env("SUPER_ADMIN_USERNAME", "admin") };
    const int user_id{ db.user_id(username) };
    if (db.is_admin(user_id)) {
        // already created
        return true;
    }

    const std::string salt{ crypto::random_string() };
    logging::debug{ "Salt: {}", salt };
    if (!db.add_super_admin(username, salt)) {
        logging::error{ R"(Fail to add super admin "{}")", username };
        return false;
    }

    return true;
}

inline void server::set_logger(httplib::Server& server, LogController& video_log_controller)
{
    server.set_logger([&video_log_controller](const httplib::Request& req, const httplib::Response& res) {
        const std::string log{ sc::log(req, res) };
        if (video_log_controller.append(log)) {
            return;
        }

        // flush video logs
        video_log_controller.flush();

        // log next log
        logging::raw_log(log);
    });
}

inline void server::set_exception_handler(httplib::Server& server)
{
    server.set_exception_handler([](const httplib::Request& /*req*/, httplib::Response& /*res*/, std::exception_ptr ep) {
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
    });
}

// Routes

namespace server
{
    // Social media bots
    inline bool is_bot(std::string user_agent)
    {
        static const std::vector<std::string> bots{
            "facebookexternalhit", "twitterbot", "linkedinbot",
            "whatsapp", "slackbot", "telegrambot", "discordbot"
        };

        su::lower(user_agent);

        return std::ranges::find_if(bots, [&user_agent](const std::string& bot) {
                   return user_agent.find(bot) != std::string::npos;
               }) != bots.cend();
    }
}

inline void server::watch_video(const httplib::Request& req, httplib::Response& res, const std::filesystem::path& bundle_dir, const Database& db)
{
    const std::string user_agent{ req.get_header_value("User-Agent") };

    if (is_bot(user_agent)) {
        const std::string video_id{ req.path_params.at("id") };
        const std::string host{ req.get_header_value("Host") };
        const std::string title{ db.video_title(su::string_to_int(video_id)) };
        static const std::string url_scheme{
#ifdef _DEBUG
            "http"
#else
            "https"
#endif
        };
        const std::string description{ "Watch " + title + " video" };
        const std::string thumbnail_url{ url_scheme + "://" + host + "/thumbnail/" + video_id };
        const std::string website_url{ url_scheme + "://" + host + "/watch-video/" + video_id };
        const std::string html{
            // clang-format off
            "<!DOCTYPE html>"
            "<html>"
            "<head>"
            "<meta property=\"og:title\" content=\"" + title + "\" />"
            "<meta property=\"og:description\" content=\"" + description + "\" />"
            "<meta property=\"og:image\" content=\"" + thumbnail_url + "\" />"
            "<meta property=\"og:url\" content=\"" + website_url + "\" />"
            "</head>"
            "<body>"
            "</body>"
            "</html>"
            // clang-format on
        };
        res.set_content(html, "text/html");
    } else {
        // normal user
        index(req, res, bundle_dir);
    }
}

inline void server::index(const httplib::Request& /*req*/, httplib::Response& res, const std::filesystem::path& bundle_dir)
{
    static const std::string index_file{ (bundle_dir / "index.html").string() };
    res.set_file_content(index_file);
}

// Auth

inline void server::refresh(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    ConnectedUser user;

    const std::string cookie{ req.get_header_value("Cookie") };
    const std::string session_id{ Session::extract_session_id_from_cookie(cookie) };
    if (session.is_valid_session(session_id)) {
        const int user_id{ su::string_to_int(session.user_from_session(session_id)) };
        const bool is_admin{ db.is_admin(user_id) };
        user.id = user_id;
        user.name = db.user_name(user_id);
        user.is_admin = is_admin;
        if (is_admin) {
            user.videos = db.admin_video_list();
        } else {
            user.videos = db.user_video_list(user_id);
        }
    } else {
        user.videos = db.no_user_video_list();
    }

    res.set_content(nlohmann::json(user).dump(), "application/json");
}

namespace server
{
    inline void register_session(const httplib::Request& req, httplib::Response& res, Session& session, int user_id)
    {
        const std::string session_id{ session.create_session(su::int_to_string(user_id)) };
        res.set_header("Set-Cookie", session.insert_session_id_to_cookie(req.get_header_value("Host"), session_id));
    }

    // Returns session_id from cookie, or empty string if not present/valid
    inline std::string session_id_from_req(const httplib::Request& req)
    {
        return Session::extract_session_id_from_cookie(req.get_header_value("Cookie"));
    }

    // Returns user_id or -1 if not authenticated
    inline int authenticated_user(const httplib::Request& req, const Session& session)
    {
        const std::string session_id{ session_id_from_req(req) };
        if (!session.is_valid_session(session_id)) {
            return -1;
        }
        return su::string_to_int(session.user_from_session(session_id));
    }

    // Returns user_id if authenticated and admin, else -1
    inline int authenticated_admin(const httplib::Request& req, const Session& session, const Database& db)
    {
        const int user_id{ authenticated_user(req, session) };
        if (user_id == -1 || !db.is_admin(user_id)) {
            return -1;
        }
        return user_id;
    }

    // Extracts all values for a given url search param form key
    // Extracts all values for a given form data key
    // Conversion [].toString => [].join(',')
    inline std::vector<int> extract_ids(const std::string& value)
    {
        return su::split(value, ',') |
               std::views::transform(su::string_to_int) |
               std::views::filter([](int i) { return i > 0; }) |
               std::ranges::to<std::vector<int>>();
    }
}

inline void server::login(const httplib::Request& req, httplib::Response& res, Session& session, const Database& db)
{
    std::string username{ req.get_param_value("username") };
    su::trim(username);
    su::lower(username);

    if (username.empty()) {
        res.status = httplib::StatusCode::NotFound_404;
        res.set_content("Empty username", "plain/text");
        return;
    }

    const int user_id{ db.user_id(username) };
    if (user_id == -1) {
        res.status = httplib::StatusCode::NotFound_404;
        res.set_content("Unknown username", "plain/text");
        return;
    }

    const std::optional db_password{ db.user_password(user_id) };
    const bool is_first_connection{ !db_password || db_password->empty() };
    if (is_first_connection) {
        res.status = httplib::StatusCode::NoContent_204;
        return;
    }

    const std::string salt{ db.user_salt(user_id) };
    const std::string password{ crypto::sha512(req.get_param_value("password")) };
    if (password.empty() || crypto::password(password, salt) != *db_password) {
        res.status = httplib::StatusCode::BadRequest_400;
        res.set_content("Invalid password", "plain/text");
        return;
    }

    register_session(req, res, session, user_id);
    res.status = httplib::StatusCode::OK_200;
}

inline void server::add_password(const httplib::Request& req, httplib::Response& res, const Database& db)
{
    std::string username{ req.get_param_value("username") };
    su::trim(username);
    su::lower(username);

    const int user_id{ db.user_id(username) };
    if (user_id == -1) {
        res.status = httplib::StatusCode::NotFound_404;
        res.set_content("Unknown username", "plain/text");
        return;
    }

    const std::optional db_password{ db.user_password(user_id) };
    const bool is_first_connection{ !db_password || db_password->empty() };
    if (!is_first_connection) {
        res.status = httplib::StatusCode::Unauthorized_401;
        res.set_content("Password already set", "plain/text");
        return;
    }

    const std::string password{ crypto::sha512(req.get_param_value("password")) };
    if (password.empty()) {
        res.status = httplib::StatusCode::NotAcceptable_406;
        res.set_content("Invalid password", "plain/text");
        return;
    }

    const std::string confirm_password{ crypto::sha512(req.get_param_value("confirmPassword")) };
    if (confirm_password != password) {
        res.status = httplib::StatusCode::Conflict_409;
        res.set_content("Passwords do not match", "plain/text");
        return;
    }

    const std::string salt{ db.user_salt(user_id) };
    const std::string new_db_password{ crypto::password(password, salt) };
    const std::optional success_user_id{ db.add_password(user_id, new_db_password) };
    if (!success_user_id.has_value()) {
        res.status = httplib::StatusCode::Locked_423;
        res.set_content("Fail to set new password", "plain/text");
        return;
    }

    logging::info{ "User password updated by {}", user_id };
    res.status = httplib::StatusCode::OK_200;
}

inline void server::logout(const httplib::Request& req, httplib::Response& res, Session& session)
{
    const std::string session_id{ session_id_from_req(req) };
    session.remove_session(session_id);
    res.status = httplib::StatusCode::OK_200;
}

inline void server::update_username(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    const int user_id{ authenticated_user(req, session) };
    if (user_id == -1) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    const std::string new_username{ req.get_param_value("username") };
    const std::string password{ crypto::sha512(req.get_param_value("password")) };

    if (new_username.empty() || password.empty()) {
        res.status = httplib::StatusCode::BadRequest_400;
        res.set_content("Missing fields", "plain/text");
        return;
    }

    const std::optional db_password{ db.user_password(user_id) };
    const std::string salt{ db.user_salt(user_id) };
    if (!db_password || crypto::password(password, salt) != *db_password) {
        res.status = httplib::StatusCode::Unauthorized_401;
        res.set_content("Invalid password", "plain/text");
        return;
    }

    const std::optional success{ db.update_username(user_id, new_username) };
    if (!success) {
        res.status = httplib::StatusCode::InternalServerError_500;
        res.set_content("Fail to update username", "plain/text");
        return;
    }

    res.status = httplib::StatusCode::OK_200;
}

inline void server::update_password(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    const int user_id{ authenticated_user(req, session) };
    if (user_id == -1) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    const std::string old_password{ crypto::sha512(req.get_param_value("oldPassword")) };
    const std::string new_password{ crypto::sha512(req.get_param_value("newPassword")) };
    const std::string confirm_password{ crypto::sha512(req.get_param_value("confirmPassword")) };

    if (old_password.empty() || new_password.empty() || confirm_password.empty()) {
        res.status = httplib::StatusCode::BadRequest_400;
        res.set_content("Missing fields", "plain/text");
        return;
    }

    const std::string salt{ db.user_salt(user_id) };
    const std::optional db_password{ db.user_password(user_id) };
    if (!db_password || crypto::password(old_password, salt) != *db_password) {
        res.status = httplib::StatusCode::Unauthorized_401;
        res.set_content("Invalid old password", "plain/text");
        return;
    }

    if (new_password != confirm_password) {
        res.status = httplib::StatusCode::Conflict_409;
        res.set_content("Passwords do not match", "plain/text");
        return;
    }

    const std::string new_db_password{ crypto::password(new_password, salt) };
    const std::optional success{ db.update_password(user_id, new_db_password) };
    if (!success) {
        res.status = httplib::StatusCode::InternalServerError_500;
        res.set_content("Fail to update password", "plain/text");
        return;
    }

    res.status = httplib::StatusCode::OK_200;
}

// Video (user)

namespace server
{
    // Check access rights
    inline bool has_video_right(const httplib::Request& req, const Session& session, const Database& db)
    {
        const std::string session_id{ session_id_from_req(req) };

        const bool is_logged{ session.is_valid_session(session_id) };
        const int video_id{ su::string_to_int(req.path_params.at("video_id")) };

        if (is_logged) {
            const int user_id{ su::string_to_int(session.user_from_session(session_id)) };
            return db.is_admin(user_id) || db.has_video_right(video_id, user_id);
        }

        return db.has_video_right(video_id);
    }

    // block video if not in watch-video page
    inline bool request_from_watch_video(const httplib::Request& req, int video_id)
    {
        const std::string referrer{ req.get_header_value("Referer") };
        return referrer.ends_with("/watch-video/" + su::int_to_string(video_id)) || referrer.ends_with("/videoserviceworker.js");
    }
}

inline void server::video_playlist(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db, VideoSession& video_session)
{
    if (!has_video_right(req, session, db)) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    const int video_id{ su::string_to_int(req.path_params.at("video_id")) };
    if (!request_from_watch_video(req, video_id)) {
        res.status = httplib::StatusCode::Forbidden_403;
        return;
    }

    const std::string playlist{ db.video_playlist(video_id) };
    if (playlist.empty()) {
        res.status = httplib::StatusCode::NotFound_404;
        return;
    }

    const std::string session_id{ session_id_from_req(req) };
    video_session.add_session(session_id, su::int_to_string(video_id));
    res.set_content(playlist, "application/vnd.apple.mpegurl");
}

inline void server::thumbnail(const httplib::Request& req, httplib::Response& res, const Database& db)
{
    const int video_id{ su::string_to_int(req.path_params.at("video_id")) };
    const std::string content{ db.thumbnail(video_id) };
    if (content.empty()) {
        res.status = httplib::StatusCode::NotFound_404;
        return;
    }

    res.set_content(content, "image/jpeg");
}

inline void server::video_segment(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db, VideoSession& video_session)
{
    if (!has_video_right(req, session, db)) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    const int video_id{ su::string_to_int(req.path_params.at("video_id")) };
    if (!request_from_watch_video(req, video_id)) {
        res.status = httplib::StatusCode::Forbidden_403;
        return;
    }

    const std::string segment{ req.path_params.at("segment") };
    const std::string session_id{ session_id_from_req(req) };

    if (!video_session.validate_segment_access(session_id, su::int_to_string(video_id), segment)) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    const std::string content{ db.video_segment(video_id, segment) };
    if (content.empty()) {
        res.status = httplib::StatusCode::NotFound_404;
        return;
    }

    res.set_content(content, "video/mp2t");
}

inline void server::start_video_session(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db, VideoSession& video_session)
{
    if (!has_video_right(req, session, db)) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    const int video_id{ su::string_to_int(req.path_params.at("video_id")) };
    if (!request_from_watch_video(req, video_id)) {
        res.status = httplib::StatusCode::Forbidden_403;
        return;
    }

    const std::string session_id{ session_id_from_req(req) };

    video_session.start_session(session_id, su::int_to_string(video_id));
    [[maybe_unused]] const bool views_incremented{ db.increment_video_views(video_id) };
    res.status = httplib::StatusCode::OK_200;
}

inline void server::reset_video_session(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db, VideoSession& video_session)
{
    if (!has_video_right(req, session, db)) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    const int video_id{ su::string_to_int(req.path_params.at("video_id")) };
    if (!request_from_watch_video(req, video_id)) {
        res.status = httplib::StatusCode::Forbidden_403;
        return;
    }

    const std::string session_id{ session_id_from_req(req) };

    video_session.reset_session(session_id, su::int_to_string(video_id));
    res.status = httplib::StatusCode::OK_200;
}

// Admin - Stats

inline void server::admin_stats(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    if (authenticated_admin(req, session, db) == -1) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    const nlohmann::json stats{
        { "userCount", db.user_count() },
        { "videoCount", db.video_count() },
        { "groupCount", db.group_count() },
        { "viewCount", db.view_count() },
    };
    res.set_content(stats.dump(), "application/json");
}

// Admin - Videos

inline void server::admin_video_list(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    if (authenticated_admin(req, session, db) == -1) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    std::vector<Video> videos{ db.admin_video_list() };
    std::vector<AdminVideoInfo> admin_videos(videos.size());
    std::ranges::transform(videos, admin_videos.begin(), [&db](const Video& video) {
        return AdminVideoInfo{
            .id = video.id,
            .title = video.title,
            .views = video.views,
            .groups = db.video_group_right_list(video.id),
            .users = db.video_user_right_list(video.id)
        };
    });

    res.set_content(nlohmann::json(admin_videos).dump(), "application/json");
}

inline void server::admin_add_video(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    if (authenticated_admin(req, session, db) == -1) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    if (!req.form.has_field("title") || !req.form.has_file("video")) {
        res.status = httplib::StatusCode::BadRequest_400;
        res.set_content("Missing fields", "plain/text");
        return;
    }

    const std::string video_title{ req.form.get_field("title") };
    const std::string video_content{ req.form.get_file("video").content };
    const std::string thumbnail_content{ video::thumbnail(video_content) };
    const std::vector<int> group_ids{ extract_ids(req.form.get_field("groupIds")) };
    const std::vector<int> user_ids{ extract_ids(req.form.get_field("userIds")) };

    const std::optional video_id{
        db.add_video(video_title, video_content)
            .and_then([&](int video_id) -> std::optional<int> {
                const std::filesystem::path video_path{ db.hls_video_path(video_id) };
                const bool converted{ video::convert_to_hls(video_content, video_path.string(), Database::hls_video_name(video_id)) };
                return converted ? std::optional(video_id) : std::nullopt;
            })
            .and_then([&](int id) -> std::optional<int> {
                return db.add_video_thumbnail(id, thumbnail_content);
            })
            .and_then([&](int id) -> std::optional<int> {
                return db.add_video_group_rights(id, group_ids) ? std::optional(id) : std::nullopt;
            })
            .and_then([&](int id) -> std::optional<int> {
                return db.add_video_user_rights(id, user_ids) ? std::optional(id) : std::nullopt;
            })
    };

    if (!video_id) {
        logging::error{ R"(Fail to add video "{}")", video_title };
        res.status = httplib::StatusCode::InternalServerError_500;
        return;
    }

    res.set_content(nlohmann::json({ { "id", *video_id } }).dump(), "application/json");
}

inline void server::admin_update_video(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    if (authenticated_admin(req, session, db) == -1) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    if (!req.has_param("title")) {
        res.status = httplib::StatusCode::BadRequest_400;
        res.set_content("Missing fields", "plain/text");
        return;
    }

    const int video_id{ su::string_to_int(req.path_params.at("video_id")) };
    const std::string video_title{ req.get_param_value("title") };
    const std::vector<int> group_ids{ extract_ids(req.get_param_value("groupIds")) };
    const std::vector<int> user_ids{ extract_ids(req.get_param_value("userIds")) };

    const std::optional success{
        db.update_video_title(video_id, video_title)
            .and_then([&](int id) -> std::optional<int> {
                return db.update_video_group_rights(id, group_ids) ? std::optional(id) : std::nullopt;
            })
            .transform([&](int id) -> bool {
                return db.update_video_user_rights(id, user_ids);
            })
    };

    if (!success.value_or(false)) {
        res.status = httplib::StatusCode::InternalServerError_500;
        return;
    }

    res.status = httplib::StatusCode::OK_200;
}

inline void server::admin_delete_video(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    if (authenticated_admin(req, session, db) == -1) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    const int video_id{ su::string_to_int(req.path_params.at("video_id")) };
    if (!db.delete_video(video_id)) {
        res.status = httplib::StatusCode::InternalServerError_500;
        return;
    }

    res.status = httplib::StatusCode::OK_200;
}

inline void server::admin_download_video(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    if (authenticated_admin(req, session, db) == -1) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    const int video_id{ su::string_to_int(req.path_params.at("video_id")) };
    const int size{ db.video_size(video_id) };
    if (size <= 0) {
        res.status = httplib::StatusCode::NotFound_404;
        return;
    }

    const std::string content{ db.video(video_id, 0, static_cast<std::size_t>(size)) };
    const std::string filename{ "video_" + su::int_to_string(video_id) + ".mp4" };
    res.set_header("Content-Disposition", "attachment; filename=\"" + filename + "\"");
    res.set_content(content, "video/mp4");
}

// Admin - Admins

inline void server::admin_admin_list(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    if (authenticated_admin(req, session, db) == -1) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    std::vector<User> admins{ db.admin_list() };
    std::vector<AdminAdminInfo> admin_admins(admins.size());
    std::ranges::transform(admins, admin_admins.begin(), [&db](const User& admin) {
        return AdminAdminInfo{
            .id = admin.id,
            .name = admin.name,
            .is_super_admin = db.is_super_admin(admin.id),
            .is_logged_once = db.user_password(admin.id).has_value()
        };
    });

    res.set_content(nlohmann::json(admin_admins).dump(), "application/json");
}

inline void server::admin_add_admin(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    if (authenticated_admin(req, session, db) == -1) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    if (!req.has_param("username")) {
        res.status = httplib::StatusCode::BadRequest_400;
        res.set_content("Missing username", "plain/text");
        return;
    }

    std::string username{ req.get_param_value("username") };
    su::trim(username);
    su::lower(username);

    // User must already exist — promote them
    const int user_id{ db.user_id(username) };
    if (user_id == -1) {
        res.status = httplib::StatusCode::NotFound_404;
        res.set_content("Unknown username", "plain/text");
        return;
    }

    if (db.is_admin(user_id)) {
        res.status = httplib::StatusCode::Conflict_409;
        res.set_content("Already admin", "plain/text");
        return;
    }

    // promote_to_admin: insert into admins table
    const std::string salt{ crypto::random_string() };
    const std::optional admin_id{ db.add_admin(username, salt) };
    if (!admin_id) {
        res.status = httplib::StatusCode::InternalServerError_500;
        return;
    }

    res.set_content(nlohmann::json({ { "id", *admin_id } }).dump(), "application/json");
}

// Admin - Users

inline void server::admin_user_list(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    if (authenticated_admin(req, session, db) == -1) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    std::vector<User> users{ db.user_list() };
    std::vector<AdminUserInfo> admin_users(users.size());
    std::ranges::transform(users, admin_users.begin(), [&db](const User& user) {
        return AdminUserInfo{
            .id = user.id,
            .name = user.name,
            .groups = db.user_group_list(user.id),
            .is_logged_once = db.user_password(user.id).has_value()
        };
    });

    res.set_content(nlohmann::json(admin_users).dump(), "application/json");
}

inline void server::admin_add_user(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    if (authenticated_admin(req, session, db) == -1) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    if (!req.has_param("username")) {
        res.status = httplib::StatusCode::BadRequest_400;
        res.set_content("Missing username", "plain/text");
        return;
    }

    std::string username{ req.get_param_value("username") };
    su::trim(username);
    su::lower(username);

    const std::string salt{ crypto::random_string() };
    const std::vector<int> group_ids{ extract_ids(req.get_param_value("groupIds")) };

    const std::optional user_id{
        db.add_user(username, salt)
            .and_then([&](int id) -> std::optional<int> {
                return db.add_user_groups(id, group_ids) ? std::optional(id) : std::nullopt;
            })
    };

    if (!user_id) {
        logging::error{ R"(Fail to add user "{}")", username };
        res.status = httplib::StatusCode::InternalServerError_500;
        return;
    }

    res.set_content(nlohmann::json({ { "id", *user_id } }).dump(), "application/json");
}

inline void server::admin_update_user(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    if (authenticated_admin(req, session, db) == -1) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    if (!req.has_param("username")) {
        res.status = httplib::StatusCode::BadRequest_400;
        res.set_content("Missing username", "plain/text");
        return;
    }

    const int user_id{ su::string_to_int(req.path_params.at("user_id")) };
    const std::string username{ req.get_param_value("username") };

    if (!db.update_username(user_id, username)) {
        res.status = httplib::StatusCode::InternalServerError_500;
        return;
    }

    const std::vector<int> group_ids{ extract_ids(req.get_param_value("groupIds")) };

    if (!group_ids.empty() && !db.update_user_groups(user_id, group_ids)) {
        res.status = httplib::StatusCode::InternalServerError_500;
        return;
    }

    res.status = httplib::StatusCode::OK_200;
}

inline void server::admin_reset_user_password(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    if (authenticated_admin(req, session, db) == -1) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    const int user_id{ su::string_to_int(req.path_params.at("user_id")) };

    if (!db.clear_password(user_id)) {
        res.status = httplib::StatusCode::InternalServerError_500;
        return;
    }

    res.status = httplib::StatusCode::OK_200;
}

inline void server::admin_delete_user(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    if (authenticated_admin(req, session, db) == -1) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    const int user_id{ su::string_to_int(req.path_params.at("user_id")) };
    if (!db.delete_user(user_id)) {
        res.status = httplib::StatusCode::InternalServerError_500;
        return;
    }

    res.status = httplib::StatusCode::OK_200;
}

// Admin - Groups

inline void server::admin_group_list(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    if (authenticated_admin(req, session, db) == -1) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    std::vector<Group> groups{ db.group_list() };
    std::vector<AdminGroupInfo> admin_groups(groups.size());
    std::ranges::transform(groups, admin_groups.begin(), [&db](const Group& group) {
        return AdminGroupInfo{
            .id = group.id,
            .name = group.name,
            .users = db.group_user_list(group.id)
        };
    });

    res.set_content(nlohmann::json(admin_groups).dump(), "application/json");
}

inline void server::admin_add_group(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    if (authenticated_admin(req, session, db) == -1) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    if (!req.has_param("name")) {
        res.status = httplib::StatusCode::BadRequest_400;
        res.set_content("Missing name", "plain/text");
        return;
    }

    const std::string group_name{ req.get_param_value("name") };
    const std::vector<int> user_ids{ extract_ids(req.get_param_value("userIds")) };

    const std::optional group_id{
        db.add_group(group_name)
            .and_then([&](int id) -> std::optional<int> {
                return db.add_group_users(id, user_ids) ? std::optional(id) : std::nullopt;
            })
    };

    if (!group_id) {
        logging::error{ R"(Fail to add group "{}")", group_name };
        res.status = httplib::StatusCode::InternalServerError_500;
        return;
    }

    res.set_content(nlohmann::json({ { "id", *group_id } }).dump(), "application/json");
}

inline void server::admin_update_group(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    if (authenticated_admin(req, session, db) == -1) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    if (!req.has_param("name")) {
        res.status = httplib::StatusCode::BadRequest_400;
        res.set_content("Missing name", "plain/text");
        return;
    }

    const int group_id{ su::string_to_int(req.path_params.at("group_id")) };
    const std::string group_name{ req.get_param_value("name") };
    const std::vector<int> user_ids{ extract_ids(req.get_param_value("userIds")) };

    const std::optional success{
        db.update_group_name(group_id, group_name)
            .transform([&](int id) -> bool {
                return db.update_group_users(id, user_ids);
            })
    };

    if (!success.value_or(false)) {
        res.status = httplib::StatusCode::InternalServerError_500;
        return;
    }

    res.status = httplib::StatusCode::OK_200;
}

inline void server::admin_delete_group(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    if (authenticated_admin(req, session, db) == -1) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    const int group_id{ su::string_to_int(req.path_params.at("group_id")) };
    if (!db.delete_group(group_id)) {
        res.status = httplib::StatusCode::InternalServerError_500;
        return;
    }

    res.status = httplib::StatusCode::OK_200;
}
