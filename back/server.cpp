#include "server.h"

#include "apimodel.h"
#include "crypto.h"
#include "database.h"
#include "env.h"
#include "filesystem.h"
#include "logging.h"
#include "servercommon.h"
#include "session.h"
#include "stringutils.h"
#include "video.h"
#include "videosession.h"

#include <httplib.h>
#include <nlohmann/json.hpp>

#include <stacktrace>

#ifdef _DEBUG // debug, allow reload of static files
#define STATIC
#else // production, don't parse multiple times
#define STATIC static
#endif

namespace server
{
    bool create_super_admin(const Database& db);

    void set_logger(httplib::Server& server);
    void set_exception_handler(httplib::Server& server);

    // Routes
    void video(const httplib::Request& req, httplib::Response& res, const std::filesystem::path& bundle_dir, const Database& db);
    void static_file(const httplib::Request& req, httplib::Response& res, const std::filesystem::path& bundle_dir);
    void index(const httplib::Request& req, httplib::Response& res, const std::filesystem::path& bundle_dir);
    void manifest(const httplib::Request& req, httplib::Response& res, const std::filesystem::path& bundle_dir);

    // Logs
    void logs(const httplib::Request& req, httplib::Response& res, logging::Logger& logger);

    // Auth
    void refresh(const httplib::Request& req, httplib::Response& res, Session& session, const Database& db);
    void login(const httplib::Request& req, httplib::Response& res, Session& session, const Database& db);
    void logout(const httplib::Request& req, httplib::Response& res, Session& session);
    void add_password(const httplib::Request& req, httplib::Response& res, const Database& db);
    void update_username(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);
    void update_password(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);

    // Video (user)
    void bookmark(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);

    void video_playlist(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);
    void video_segment(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db, VideoSession& video_session);
    void thumbnail(const httplib::Request& req, httplib::Response& res, const Database& db);
    void add_video_session(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db, VideoSession& video_session);
    void start_video_session(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db, VideoSession& video_session);
    void reset_video_session(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db, VideoSession& video_session);

    // Admin - stats
    void admin_stats(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);

    // Admin - videos
    void admin_video_list(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);
    void admin_video(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);
    void admin_add_video(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);
    void admin_update_video(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);
    void admin_delete_video(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);
    void admin_download_video(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);

    void admin_location_list(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);
    void admin_add_location(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);
    void admin_delete_location(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);

    void admin_author_list(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);
    void admin_add_author(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);
    void admin_delete_author(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);

    void admin_tag_list(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);
    void admin_add_tag(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);
    void admin_delete_tag(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);

    // Admin - admins
    void admin_admin_list(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);
    void admin_admin(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);
    void admin_add_admin(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);
    void admin_update_admin(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);

    // Admin - users
    void admin_user_list(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);
    void admin_user(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);
    void admin_add_user(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);
    void admin_update_user(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);
    void admin_deactivate_user(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);
    void admin_reset_user_password(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);
    void admin_delete_user(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);

    // Admin - groups
    void admin_group_list(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);
    void admin_group(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db);
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

    // database session management

    session.add_insert_function([&db](const std::string& session_id, int user_id, const std::string& creation_date, const std::string& max_age_time) {
        db.add_session(session_id, user_id, creation_date, max_age_time);
    });

    session.add_remove_function([&db](const std::string& session_id) {
        if (!db.delete_session(session_id)) {
            const std::optional session{ db.session(session_id) };
            if (!session) {
                logging::error{ R"(Fail to delete session: unknown session "{}")", session_id };
                return;
            }

            const std::optional user{ db.user(session->user_id) };
            if (!user) {
                logging::error{ R"(Fail to delete session: unknown user "{}" in session "{}")", session->user_id, session_id };
                return;
            }

            logging::error{ R"(Fail to delete session "{}" of user "{}")", session_id, user->name };
        }
    });

    // init after added both functions to cleanup expired sessions
    session.init_from_map(db.session_list());

    const std::filesystem::path logs_path{ filesystem::logs_path() / "front.log" };
    logging::Logger front_logger;
    front_logger.open(logs_path);

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

    set_logger(server);
    set_exception_handler(server);

    server
        .Get("/video/:id", sc::serve(video, std::cref(bundle_dir), std::cref(db)))
        .Get(R"(.*\/manifest\.json$)", sc::serve(manifest, std::cref(bundle_dir)))
        .Get(R"((?!\/api\/).*\.[^/]+$)", sc::serve(static_file, std::cref(bundle_dir)))
        .Get(R"((?!\/api\/).*)", sc::serve(index, std::cref(bundle_dir)))

        .Post("/api/logs", sc::serve(logs, std::ref(front_logger)))

        .Get("/api/refresh", sc::serve(refresh, std::ref(session), std::cref(db)))
        .Post("/api/login", sc::serve(login, std::ref(session), std::cref(db)))
        .Post("/api/logout", sc::serve(logout, std::ref(session)))
        .Post("/api/add-password", sc::serve(add_password, std::cref(db)))
        .Post("/api/update-username", sc::serve(update_username, std::cref(session), std::cref(db)))
        .Post("/api/update-password", sc::serve(update_password, std::cref(session), std::cref(db)))

        .Post("/api/bookmark/:video_id", sc::serve(bookmark, std::cref(session), std::cref(db)))

        .Get("/api/thumbnail/:video_id", sc::serve(thumbnail, std::cref(db)))
        .Get("/api/video/:video_id/playlist", sc::serve(video_playlist, std::cref(session), std::cref(db)))
        .Get("/api/video/:video_id/:segment", sc::serve(video_segment, std::cref(session), std::cref(db), std::ref(video_session)))
        .Post("/api/add-video-session/:video_id", sc::serve(add_video_session, std::cref(session), std::cref(db), std::ref(video_session)))
        .Post("/api/start-video-session/:video_id", sc::serve(start_video_session, std::cref(session), std::cref(db), std::ref(video_session)))
        .Post("/api/reset-video-session/:video_id", sc::serve(reset_video_session, std::cref(session), std::cref(db), std::ref(video_session)))

        .Get("/api/admin/stats", sc::serve(admin_stats, std::cref(session), std::cref(db)))

        .Get("/api/admin/video-list", sc::serve(admin_video_list, std::cref(session), std::cref(db)))
        .Get("/api/admin/video/:video_id", sc::serve(admin_video, std::cref(session), std::cref(db)))
        .Post("/api/admin/add-video", sc::serve(admin_add_video, std::cref(session), std::cref(db)))
        .Post("/api/admin/update-video/:video_id", sc::serve(admin_update_video, std::cref(session), std::cref(db)))
        .Post("/api/admin/delete-video/:video_id", sc::serve(admin_delete_video, std::cref(session), std::cref(db)))
        .Get("/api/admin/download-video/:video_id", sc::serve(admin_download_video, std::cref(session), std::cref(db)))

        .Get("/api/admin/location-list", sc::serve(admin_location_list, std::cref(session), std::cref(db)))
        .Post("/api/admin/add-location", sc::serve(admin_add_location, std::cref(session), std::cref(db)))
        .Post("/api/admin/delete-location/:location_id", sc::serve(admin_delete_location, std::cref(session), std::cref(db)))

        .Get("/api/admin/author-list", sc::serve(admin_author_list, std::cref(session), std::cref(db)))
        .Post("/api/admin/add-author", sc::serve(admin_add_author, std::cref(session), std::cref(db)))
        .Post("/api/admin/delete-author/:author_id", sc::serve(admin_delete_author, std::cref(session), std::cref(db)))

        .Get("/api/admin/tag-list", sc::serve(admin_tag_list, std::cref(session), std::cref(db)))
        .Post("/api/admin/add-tag", sc::serve(admin_add_tag, std::cref(session), std::cref(db)))
        .Post("/api/admin/delete-tag/:tag_id", sc::serve(admin_delete_tag, std::cref(session), std::cref(db)))

        .Get("/api/admin/admin-list", sc::serve(admin_admin_list, std::cref(session), std::cref(db)))
        .Get("/api/admin/admin/:admin_id", sc::serve(admin_admin, std::cref(session), std::cref(db)))
        .Post("/api/admin/add-admin", sc::serve(admin_add_admin, std::cref(session), std::cref(db)))
        .Post("/api/admin/update-admin/:admin_id", sc::serve(admin_update_admin, std::cref(session), std::cref(db)))

        .Get("/api/admin/user-list", sc::serve(admin_user_list, std::cref(session), std::cref(db)))
        .Get("/api/admin/user/:user_id", sc::serve(admin_user, std::cref(session), std::cref(db)))
        .Post("/api/admin/add-user", sc::serve(admin_add_user, std::cref(session), std::cref(db)))
        .Post("/api/admin/update-user/:user_id", sc::serve(admin_update_user, std::cref(session), std::cref(db)))
        .Post("/api/admin/deactivate-user/:user_id", sc::serve(admin_deactivate_user, std::cref(session), std::cref(db)))
        .Post("/api/admin/reset-user-password/:user_id", sc::serve(admin_reset_user_password, std::cref(session), std::cref(db)))
        .Post("/api/admin/delete-user/:user_id", sc::serve(admin_delete_user, std::cref(session), std::cref(db)))

        .Get("/api/admin/group-list", sc::serve(admin_group_list, std::cref(session), std::cref(db)))
        .Get("/api/admin/group/:group_id", sc::serve(admin_group, std::cref(session), std::cref(db)))
        .Post("/api/admin/add-group", sc::serve(admin_add_group, std::cref(session), std::cref(db)))
        .Post("/api/admin/update-group/:group_id", sc::serve(admin_update_group, std::cref(session), std::cref(db)))
        .Post("/api/admin/delete-group/:group_id", sc::serve(admin_delete_group, std::cref(session), std::cref(db)));

    static const int port{ su::string_to_int(env::back_port) };
    logging::info{ "Serving HTTP on http://{0}:{1} ...", env::back_host, port };
    return (server.listen(env::back_host, port) ? EXIT_SUCCESS : EXIT_FAILURE);
}

inline bool server::create_super_admin(const Database& db)
{
    const std::optional user{ db.user(env::super_admin_username) };
    if (user && db.is_admin(user->id)) {
        // already created
        return true;
    }

    const std::string salt{ crypto::random_string() };
    logging::debug{ "Salt: {}", salt };
    if (!db.add_super_admin(env::super_admin_username, salt)) {
        logging::error{ R"(Fail to add super admin "{}")", env::super_admin_username };
        return false;
    }

    return true;
}

inline void server::set_logger(httplib::Server& server)
{
    server.set_logger([](const httplib::Request& req, const httplib::Response& res) {
        const std::string log{ sc::log(req, res) };
        logging::raw_log(log);
    });
}

inline void server::set_exception_handler(httplib::Server& server)
{
    server.set_exception_handler([](const httplib::Request& /*req*/, httplib::Response& res, std::exception_ptr ep) {
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
        res.set_content(message, "text/plain");
        res.status = httplib::StatusCode::InternalServerError_500;
    });
}

// Routes

namespace server
{
    // Social media bots
    inline bool is_bot(const std::string& user_agent)
    {
        static constexpr std::array bots{
            "facebookexternalhit", "twitterbot", "linkedinbot",
            "whatsapp", "slackbot", "telegrambot", "discordbot",
            "skype", "redditbot", "mastodon", "friendica", "bluesky"
        };

        const std::string u{ su::lower(user_agent) };

        return std::ranges::find_if(bots, [&u](const auto& bot) {
                   return u.find(bot) != std::string::npos;
               }) != bots.cend();
    }

    struct IndexMetaData
    {
        std::string title;
        std::string description;
        std::string image;
        std::string url;
    };

    inline void serve_index(const httplib::Request& req, httplib::Response& res, const std::filesystem::path& bundle_dir, const IndexMetaData& metadata)
    {
        const std::string user_agent{ req.get_header_value("User-Agent") };

        if (is_bot(user_agent)) {
            static const std::string url_scheme{
#ifdef _DEBUG
                "http"
#else
                "https"
#endif
            };
            const std::string host{ req.get_header_value("Host") };
            const std::string base_url{ url_scheme + "://" + host };
            const std::string html{ std::format(
                "<!DOCTYPE html>"
                "<html>"
                "<head>"
                "<meta property=\"og:title\" content=\"{}\" />"
                "<meta property=\"og:description\" content=\"{}\" />"
                "<meta property=\"og:image\" content=\"{}\" />"
                "<meta property=\"og:url\" content=\"{}\" />"
                "</head>"
                "<body>"
                "</body>"
                "</html>",
                metadata.title,
                metadata.description,
                base_url + metadata.image,
                base_url + metadata.url) };
            res.set_content(html, "text/html");
        } else {
            // normal user
            STATIC const std::string index_content{
                [&bundle_dir] {
                    const nlohmann::json env_json{
                        { "websiteName", env::website_name },
                    };

                    const std::filesystem::path index_file{ bundle_dir / "index.html" };
                    const std::string index_content{ filesystem::read_file(index_file) };

                    const std::regex env_regex{ "__ENV_PLACEHOLDER__" };
                    // replace all occurrences of envPattern with env_json.dump()
                    return std::regex_replace(index_content, env_regex, env_json.dump());
                }()
            };
            res.set_content(index_content, "text/html");
        }
    }
}

inline void server::video(const httplib::Request& req, httplib::Response& res, const std::filesystem::path& bundle_dir, const Database& db)
{
    const std::string video_id{ req.path_params.at("id") };
    const std::optional video{ db.video(su::string_to_int(video_id)) };
    if (!video) {
        res.status = httplib::StatusCode::NotFound_404;
        res.set_content("Unknown video", "plain/text");
        return;
    }

    const std::string description{ "Watch " + video->title + " video" };
    const std::string thumbnail_url{ "/api/thumbnail/" + video_id };
    const std::string video_url{ "/video/" + video_id };
    serve_index(req, res, bundle_dir,
                { .title = video->title,
                  .description = description,
                  .image = thumbnail_url,
                  .url = video_url });
}

inline void server::static_file(const httplib::Request& req, httplib::Response& res, const std::filesystem::path& bundle_dir)
{
    STATIC const std::map bundle_files{
        [&bundle_dir] {
            std::map<std::string, std::string> files;
            for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator(bundle_dir)) {
                if (entry.is_regular_file() && entry.path().filename() != "index.html") {
                    const std::string rel{ "/" + std::filesystem::relative(entry.path(), bundle_dir).string() };
                    files.emplace(rel, entry.path().string());
                }
            }
            return files;
        }()
    };

    const auto bundle_file_it{ bundle_files.find(req.path) };
    if (bundle_file_it == bundle_files.end()) {
        logging::error{ "File not found: {}", req.path };
        res.status = httplib::StatusCode::NotFound_404;
        return;
    }

    res.set_file_content(bundle_file_it->second);
}

inline void server::index(const httplib::Request& req, httplib::Response& res, const std::filesystem::path& bundle_dir)
{
    const std::string title{ env::website_name };
    const std::string description{ "Welcome to " + env::website_name + " home" };
    const std::string icon_url{ "/assets/icons/icon.png" };
    const std::string website_url{ "/" };
    serve_index(req, res, bundle_dir,
                { .title = title,
                  .description = description,
                  .image = icon_url,
                  .url = website_url });
}

inline void server::manifest(const httplib::Request& /*req*/, httplib::Response& res, const std::filesystem::path& bundle_dir)
{
    STATIC const std::string manifest_content{
        [&bundle_dir] {
            const std::filesystem::path manifest_file{ bundle_dir / "manifest.json" };
            const std::string manifest_content{ filesystem::read_file(manifest_file) };
            nlohmann::json manifest = nlohmann::json::parse(manifest_content);
            manifest.update(nlohmann::json{
                { "name", env::website_name },
                { "short_name", env::short_website_name } });
            return manifest.dump();
        }()
    };
    res.set_content(manifest_content, "application/json");
}

// Logs

inline void server::logs(const httplib::Request& req, httplib::Response& res, logging::Logger& logger)
{
    const nlohmann::json json = nlohmann::json::parse(req.body);
    if (!json.is_array()) {
        res.status = httplib::StatusCode::BadRequest_400;
        return;
    }

    for (const auto& log_entry : json) {
        const std::string level{ log_entry.value("level", "log") };
        const std::string message{ log_entry.value("message", "") };
        const std::string timestamp{ log_entry.value("timestamp", "") };
        const std::string host{ log_entry.value("host", "?") };
        const std::string user_agent{ log_entry.value("userAgent", "?") };
        const std::string path{ log_entry.value("path", "/") };

        std::string log_message{ std::format("{} - {} - {} - {} - {} - {}", level, timestamp, host, path, message, user_agent) };
        if (level == "error") {
            const std::string function{ log_entry.value("function", "anonymous_function") };
            const std::string file{ log_entry.value("file", "?") };
            const int line{ log_entry.value("line", 0) };
            log_message += std::format(" - {} ({}:{})", function, file, line);
        }

        logger << log_message << '\n';
    }
}

// Auth

namespace server
{
    // Returns session_id from cookie, or empty string if not present/valid
    inline std::string session_id_from_req(const httplib::Request& req)
    {
        return Session::extract_session_id_from_cookie(req.get_header_value("Cookie"));
    }

    // Returns user_id or Session::invalid_user_id() if not authenticated
    inline int authenticated_user(const httplib::Request& req, const Session& session)
    {
        const std::string session_id{ session_id_from_req(req) };
        return session.user_from_session(session_id);
    }

    // Returns user_id if authenticated and admin, else Session::invalid_user_id()
    inline int authenticated_admin(const httplib::Request& req, const Session& session, const Database& db)
    {
        const int user_id{ authenticated_user(req, session) };
        if (user_id == Session::invalid_user_id() || !db.is_admin(user_id)) {
            return Session::invalid_user_id();
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

    inline std::optional<std::string> location_id_to_name(const Database& db, const std::optional<int>& location_id)
    {
        return location_id
            .and_then([&db](int id) { return db.location(id); })
            .transform([](const Location& location) { return location.name; });
    }

    template <typename T, typename P>
    inline auto types_to_elements(const std::vector<T>& types, P member)
    {
        return types |
               std::views::transform([member](const T& type) { return type.*member; }) |
               std::ranges::to<std::vector<std::remove_cvref_t<decltype(std::declval<T>().*member)>>>();
    }

    // Convert when not connected
    inline std::vector<VideoInfo> video_to_video_info(const Database& db, const std::vector<Video>& videos)
    {
        std::vector<VideoInfo> video_infos(videos.size());
        std::ranges::transform(videos, video_infos.begin(), [&db](const Video& video) -> VideoInfo {
            return VideoInfo{
                .id = video.id,
                .title = video.title,
                .date = video.date,
                .location = location_id_to_name(db, video.location_id),
                .authors = types_to_elements(db.video_author_list(video.id), &Author::name),
                .tags = types_to_elements(db.video_tag_list(video.id), &Tag::name)
            };
        });
        return video_infos;
    }

    // Convert when connected
    inline std::vector<VideoInfo> video_to_video_info(int user_id, const Database& db, const std::vector<Video>& videos)
    {
        std::vector<VideoInfo> video_infos(videos.size());
        std::ranges::transform(videos, video_infos.begin(), [&user_id, &db](const Video& video) -> VideoInfo {
            return VideoInfo{
                .id = video.id,
                .title = video.title,
                .bookmarked = db.bookmarked(user_id, video.id),
                .date = video.date,
                .location = location_id_to_name(db, video.location_id),
                .authors = types_to_elements(db.video_author_list(video.id), &Author::name),
                .tags = types_to_elements(db.video_tag_list(video.id), &Tag::name)
            };
        });
        return video_infos;
    }
}

inline void server::refresh(const httplib::Request& req, httplib::Response& res, Session& session, const Database& db)
{
    ConnectedUser user;

    const std::string session_id{ session_id_from_req(req) };
    const int user_id{ session.user_from_session(session_id) };
    if (user_id != Session::invalid_user_id()) {
        const std::optional db_user{ db.user(user_id) };
        if (!db_user) {
            res.status = httplib::StatusCode::NotFound_404;
            res.set_content("Unknown user", "plain/text");
            return;
        }

        // reset session
        res.set_header("Set-Cookie", session.insert_session_id_to_cookie(session_id));

        const bool is_admin{ db.is_admin(user_id) };
        user.id = user_id;
        user.name = db_user->name;
        user.is_admin = is_admin;
        if (is_admin) {
            user.videos = video_to_video_info(user_id, db, db.admin_video_list());
        } else {
            user.videos = video_to_video_info(user_id, db, db.user_video_list(user_id));
        }
    } else {
        user.videos = video_to_video_info(db, db.no_user_video_list());
    }

    res.set_content(nlohmann::json(user).dump(), "application/json");
}

inline void server::login(const httplib::Request& req, httplib::Response& res, Session& session, const Database& db)
{
    const std::string username{ su::trim(req.get_param_value("username")) };
    if (username.empty()) {
        res.status = httplib::StatusCode::NotFound_404;
        res.set_content("Empty username", "plain/text");
        return;
    }

    const std::optional user{ db.user(username) };
    if (!user) {
        res.status = httplib::StatusCode::NotFound_404;
        res.set_content("Unknown username", "plain/text");
        return;
    }

    const bool is_first_connection{ !user->password || user->password->empty() };
    if (is_first_connection) {
        res.status = httplib::StatusCode::NoContent_204;
        return;
    }

    const std::string password{ crypto::sha512(req.get_param_value("password")) };
    if (password.empty() || crypto::password(password, user->salt) != *user->password) {
        res.status = httplib::StatusCode::BadRequest_400;
        res.set_content("Invalid password", "plain/text");
        return;
    }

    const bool deactivated{ db.deactivated_user(user->id) };
    if (deactivated) {
        res.status = httplib::StatusCode::Locked_423;
        res.set_content("Deactivated user", "plain/text");
        return;
    }

    // create session
    const std::string session_id{ session.create_session(user->id) };
    res.set_header("Set-Cookie", session.insert_session_id_to_cookie(session_id));

    res.status = httplib::StatusCode::OK_200;
}

inline void server::add_password(const httplib::Request& req, httplib::Response& res, const Database& db)
{
    const std::string username{ su::trim(req.get_param_value("username")) };
    const std::optional user{ db.user(username) };
    if (!user) {
        res.status = httplib::StatusCode::NotFound_404;
        res.set_content("Unknown username", "plain/text");
        return;
    }

    const bool is_first_connection{ !user->password || user->password->empty() };
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

    const std::string new_db_password{ crypto::password(password, user->salt) };
    const std::optional success_user_id{ db.add_password(user->id, new_db_password) };
    if (!success_user_id.has_value()) {
        res.status = httplib::StatusCode::Locked_423;
        res.set_content("Fail to set new password", "plain/text");
        return;
    }

    logging::info{ "User password updated by {}", user->id };
    res.status = httplib::StatusCode::OK_200;
}

inline void server::logout(const httplib::Request& req, httplib::Response& res, Session& session)
{
    const std::string session_id{ session_id_from_req(req) };
    res.set_header("Set-Cookie", session.remove_session_reset_cookie(session_id));
    res.status = httplib::StatusCode::OK_200;
}

namespace server
{
    inline bool validate_field(httplib::Response& res, const std::string& field)
    {
        static const std::regex allow_list(R"(^[a-zA-Z0-9\s.,!?'"()\-_:;\x80-\xFF]*$)");
        if (std::regex_match(field, allow_list)) {
            return true;
        }

        res.status = httplib::StatusCode::BadRequest_400;
        res.set_content("Unauthorized character set", "plain/text");
        return false;
    }
}

inline void server::update_username(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    const int user_id{ authenticated_user(req, session) };
    const std::optional user{ db.user(user_id) };
    if (!user) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    const std::string username{ su::trim(req.get_param_value("username")) };
    const std::string password{ crypto::sha512(req.get_param_value("password")) };

    if (username.empty()) {
        res.status = httplib::StatusCode::BadRequest_400;
        res.set_content("Missing username field", "plain/text");
        return;
    }

    if (!validate_field(res, username)) {
        return;
    }

    if (password.empty()) {
        res.status = httplib::StatusCode::BadRequest_400;
        res.set_content("Missing password field", "plain/text");
        return;
    }

    if (db.user_exists(user_id, username)) {
        res.status = httplib::StatusCode::Conflict_409;
        res.set_content("Username already exists", "plain/text");
        return;
    }

    if (!user->password || crypto::password(password, user->salt) != *user->password) {
        res.status = httplib::StatusCode::Unauthorized_401;
        res.set_content("Invalid password", "plain/text");
        return;
    }

    const std::optional success{ db.update_username(user_id, username) };
    if (!success) {
        res.status = httplib::StatusCode::InternalServerError_500;
        res.set_content("Fail to update username", "plain/text");
        logging::error{ R"(Fail to update username "{}")", user->name };
        return;
    }

    res.status = httplib::StatusCode::OK_200;
}

inline void server::update_password(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    const int user_id{ authenticated_user(req, session) };
    const std::optional user{ db.user(user_id) };
    if (!user) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    const std::string old_password{ crypto::sha512(req.get_param_value("oldPassword")) };
    const std::string new_password{ crypto::sha512(req.get_param_value("newPassword")) };
    const std::string confirm_password{ crypto::sha512(req.get_param_value("confirmPassword")) };

    if (old_password.empty() || new_password.empty() || confirm_password.empty()) {
        res.status = httplib::StatusCode::BadRequest_400;
        res.set_content("Missing password fields", "plain/text");
        return;
    }

    if (!user->password || crypto::password(old_password, user->salt) != *user->password) {
        res.status = httplib::StatusCode::Unauthorized_401;
        res.set_content("Invalid old password", "plain/text");
        return;
    }

    if (new_password != confirm_password) {
        res.status = httplib::StatusCode::Conflict_409;
        res.set_content("Passwords do not match", "plain/text");
        return;
    }

    const std::string new_db_password{ crypto::password(new_password, user->salt) };
    const std::optional success{ db.update_password(user_id, new_db_password) };
    if (!success) {
        res.status = httplib::StatusCode::InternalServerError_500;
        res.set_content("Fail to update password", "plain/text");
        logging::error{ R"(Fail to update user password "{}")", user->name };
        return;
    }

    res.status = httplib::StatusCode::OK_200;
}

// Video (user)

inline void server::bookmark(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    const int user_id{ authenticated_user(req, session) };
    if (user_id == Session::invalid_user_id()) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    const int video_id{ su::string_to_int(req.path_params.at("video_id")) };
    const bool bookmarked{ su::string_to_bool(req.get_param_value("bookmarked")) };

    const bool success{ db.set_bookmark(user_id, video_id, bookmarked) };
    if (!success) {
        res.status = httplib::StatusCode::InternalServerError_500;
        res.set_content("Fail to set the bookmark", "plain/text");
        logging::error{ R"(Fail to set bookmark "{}" for user "{}" and video "{}")", bookmarked, user_id, video_id };
        return;
    }

    res.status = httplib::StatusCode::OK_200;
}

namespace server
{
    // Check access rights
    inline bool has_video_right(const httplib::Request& req, const Session& session, const Database& db)
    {
        const std::string session_id{ session_id_from_req(req) };

        const int user_id{ session.user_from_session(session_id) };
        const int video_id{ su::string_to_int(req.path_params.at("video_id")) };

        if (user_id != Session::invalid_user_id()) {
            return db.is_admin(user_id) || db.has_video_right(video_id, user_id);
        }

        return db.has_video_right(video_id);
    }

    // block video if not in video page
    inline bool request_from_video(const httplib::Request& req, int video_id)
    {
        const std::string referrer{ req.get_header_value("Referer") };
        return referrer.ends_with("/video/" + su::int_to_string(video_id));
    }
}

inline void server::video_playlist(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    if (!has_video_right(req, session, db)) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    const int video_id{ su::string_to_int(req.path_params.at("video_id")) };
    if (!request_from_video(req, video_id)) {
        res.status = httplib::StatusCode::Forbidden_403;
        return;
    }

    const std::string playlist{ db.video_playlist(video_id) };
    if (playlist.empty()) {
        res.status = httplib::StatusCode::NotFound_404;
        return;
    }

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
    if (!request_from_video(req, video_id)) {
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

inline void server::add_video_session(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db, VideoSession& video_session)
{
    if (!has_video_right(req, session, db)) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    const int video_id{ su::string_to_int(req.path_params.at("video_id")) };
    if (!request_from_video(req, video_id)) {
        res.status = httplib::StatusCode::Forbidden_403;
        return;
    }

    const std::string session_id{ session_id_from_req(req) };

    video_session.add_session(session_id, su::int_to_string(video_id));
    res.status = httplib::StatusCode::OK_200;
}

inline void server::start_video_session(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db, VideoSession& video_session)
{
    if (!has_video_right(req, session, db)) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    const int video_id{ su::string_to_int(req.path_params.at("video_id")) };
    if (!request_from_video(req, video_id)) {
        res.status = httplib::StatusCode::Forbidden_403;
        return;
    }

    const std::string session_id{ session_id_from_req(req) };

    video_session.start_session(session_id, su::int_to_string(video_id));
    res.status = httplib::StatusCode::OK_200;
}

inline void server::reset_video_session(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db, VideoSession& video_session)
{
    if (!has_video_right(req, session, db)) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    const int video_id{ su::string_to_int(req.path_params.at("video_id")) };
    if (!request_from_video(req, video_id)) {
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
    if (authenticated_admin(req, session, db) == Session::invalid_user_id()) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    const nlohmann::json stats{
        { "userCount", db.user_count() },
        { "videoCount", db.video_count() },
        { "groupCount", db.group_count() },
    };
    res.set_content(stats.dump(), "application/json");
}

// Admin - Videos

inline void server::admin_video_list(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    if (authenticated_admin(req, session, db) == Session::invalid_user_id()) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    std::vector<Video> videos{ db.admin_video_list() };
    std::vector<AdminVideoInfo> admin_videos(videos.size());
    std::ranges::transform(videos, admin_videos.begin(), [&db](const Video& video) {
        return AdminVideoInfo{
            .id = video.id,
            .title = video.title,
            .date = video.date,
            .location = video.location_id.and_then([&db](int location_id) { return db.location(location_id); }),
            .authors = db.video_author_list(video.id),
            .tags = db.video_tag_list(video.id),
            .groups = db.video_group_right_list(video.id),
            .users = db.video_user_right_list(video.id)
        };
    });

    res.set_content(nlohmann::json(admin_videos).dump(), "application/json");
}

inline void server::admin_video(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    if (authenticated_admin(req, session, db) == Session::invalid_user_id()) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    const int video_id{ su::string_to_int(req.path_params.at("video_id")) };
    const std::optional video{ db.video(video_id) };
    if (!video) {
        res.status = httplib::StatusCode::NotFound_404;
        return;
    }

    const AdminVideoInfo admin_video{
        .id = video_id,
        .title = video->title,
        .date = video->date,
        .location = video->location_id.and_then([&db](int location_id) { return db.location(location_id); }),
        .authors = db.video_author_list(video_id),
        .tags = db.video_tag_list(video_id),
        .groups = db.video_group_right_list(video_id),
        .users = db.video_user_right_list(video_id)
    };

    res.set_content(nlohmann::json(admin_video).dump(), "application/json");
}

inline void server::admin_add_video(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    if (authenticated_admin(req, session, db) == Session::invalid_user_id()) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    if (!req.form.has_field("title")) {
        res.status = httplib::StatusCode::BadRequest_400;
        res.set_content("Missing title field", "plain/text");
        return;
    }

    if (!req.form.has_file("video")) {
        res.status = httplib::StatusCode::BadRequest_400;
        res.set_content("Missing video field", "plain/text");
        return;
    }

    const std::string video_title{ su::trim(req.form.get_field("title")) };
    const std::string video_date{ su::trim(req.form.get_field("date")) };
    const std::optional video_opt_date{ video_date.empty() ? std::nullopt : std::optional(video_date) };
    const int location_id{ su::string_to_int(req.form.get_field("locationId")) };
    const std::optional location_opt_id{ location_id == 0 ? std::nullopt : std::optional(location_id) };
    const std::string video_content{ req.form.get_file("video").content };
    const std::string thumbnail_content{ video::thumbnail(video_content) };
    const std::vector author_ids{ extract_ids(req.form.get_field("authorIds")) };
    const std::vector tag_ids{ extract_ids(req.form.get_field("tagIds")) };
    const std::vector group_ids{ extract_ids(req.form.get_field("groupIds")) };
    const std::vector user_ids{ extract_ids(req.form.get_field("userIds")) };

    if (!validate_field(res, video_title)) {
        return;
    }

    const std::optional video_id{
        db.add_video(video_title, video_opt_date, location_opt_id, video_content)
            .and_then([&](int video_id) -> std::optional<int> {
                const std::filesystem::path video_path{ db.hls_video_path(video_id) };
                const bool converted{ video::convert_to_hls(video_content, video_path.string(), Database::hls_video_name(video_id)) };
                return converted ? std::optional(video_id) : std::nullopt;
            })
            .and_then([&](int id) -> std::optional<int> {
                return db.add_video_thumbnail(id, thumbnail_content);
            })
            .and_then([&](int id) -> std::optional<int> {
                return db.add_video_authors(id, author_ids) ? std::optional(id) : std::nullopt;
            })
            .and_then([&](int id) -> std::optional<int> {
                return db.add_video_tags(id, tag_ids) ? std::optional(id) : std::nullopt;
            })
            .and_then([&](int id) -> std::optional<int> {
                return db.add_video_group_rights(id, group_ids) ? std::optional(id) : std::nullopt;
            })
            .and_then([&](int id) -> std::optional<int> {
                return db.add_video_user_rights(id, user_ids) ? std::optional(id) : std::nullopt;
            })
    };

    if (!video_id) {
        res.status = httplib::StatusCode::InternalServerError_500;
        res.set_content("Fail to add the video", "plain/text");
        logging::error{ R"(Fail to add video "{}")", video_title };
        return;
    }

    res.set_content(nlohmann::json({ { "id", *video_id } }).dump(), "application/json");
}

inline void server::admin_update_video(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    if (authenticated_admin(req, session, db) == Session::invalid_user_id()) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    if (!req.has_param("title")) {
        res.status = httplib::StatusCode::BadRequest_400;
        res.set_content("Missing title field", "plain/text");
        return;
    }

    const int video_id{ su::string_to_int(req.path_params.at("video_id")) };
    const std::string video_title{ su::trim(req.get_param_value("title")) };
    const std::string video_date{ su::trim(req.get_param_value("date")) };
    const std::optional video_opt_date{ video_date.empty() ? std::nullopt : std::optional(video_date) };
    const int location_id{ su::string_to_int(req.get_param_value("locationId")) };
    const std::optional location_opt_id{ location_id == 0 ? std::nullopt : std::optional(location_id) };
    const std::vector author_ids{ extract_ids(req.get_param_value("authorIds")) };
    const std::vector tag_ids{ extract_ids(req.get_param_value("tagIds")) };
    const std::vector<int> group_ids{ extract_ids(req.get_param_value("groupIds")) };
    const std::vector<int> user_ids{ extract_ids(req.get_param_value("userIds")) };

    if (!validate_field(res, video_title)) {
        return;
    }

    const std::optional success{
        db.update_video(video_id, video_title, video_opt_date, location_opt_id)
            .and_then([&](int id) -> std::optional<int> {
                return db.update_video_authors(id, author_ids) ? std::optional(id) : std::nullopt;
            })
            .and_then([&](int id) -> std::optional<int> {
                return db.update_video_tags(id, tag_ids) ? std::optional(id) : std::nullopt;
            })
            .and_then([&](int id) -> std::optional<int> {
                return db.update_video_group_rights(id, group_ids) ? std::optional(id) : std::nullopt;
            })
            .transform([&](int id) -> bool {
                return db.update_video_user_rights(id, user_ids);
            })
    };

    if (!success.value_or(false)) {
        res.status = httplib::StatusCode::InternalServerError_500;
        res.set_content("Fail to update the video", "plain/text");
        logging::error{ R"(Fail to update video "{}")", video_id };
        return;
    }

    res.status = httplib::StatusCode::OK_200;
}

inline void server::admin_delete_video(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    if (authenticated_admin(req, session, db) == Session::invalid_user_id()) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    const int video_id{ su::string_to_int(req.path_params.at("video_id")) };
    if (!db.delete_video(video_id)) {
        res.status = httplib::StatusCode::InternalServerError_500;
        logging::error{ R"(Fail to delete video "{}")", video_id };
        return;
    }

    res.status = httplib::StatusCode::OK_200;
}

inline void server::admin_download_video(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    if (authenticated_admin(req, session, db) == Session::invalid_user_id()) {
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
    res.set_content(content, "video/mp4");
}

inline void server::admin_location_list(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    if (authenticated_admin(req, session, db) == Session::invalid_user_id()) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    const std::vector locations{ db.location_list() };
    res.set_content(nlohmann::json(locations).dump(), "application/json");
}

inline void server::admin_add_location(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    if (authenticated_admin(req, session, db) == Session::invalid_user_id()) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    const std::string location{ su::trim(req.get_param_value("name")) };

    if (!validate_field(res, location)) {
        return;
    }

    if (db.location_exists(location)) {
        res.status = httplib::StatusCode::Conflict_409;
        res.set_content("Location already exists", "plain/text");
        return;
    }

    const std::optional location_id{ db.add_location(location) };
    if (!location_id) {
        res.status = httplib::StatusCode::InternalServerError_500;
        res.set_content("Fail to add the location", "plain/text");
        logging::error{ R"(Fail to add location "{}")", location };
        return;
    }

    res.set_content(nlohmann::json({ { "id", *location_id } }).dump(), "application/json");
}

inline void server::admin_delete_location(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    if (authenticated_admin(req, session, db) == Session::invalid_user_id()) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    const int location_id{ su::string_to_int(req.path_params.at("location_id")) };
    if (!db.delete_location(location_id)) {
        res.status = httplib::StatusCode::InternalServerError_500;
        logging::error{ R"(Fail to delete location "{}")", location_id };
        return;
    }

    res.status = httplib::StatusCode::OK_200;
}

inline void server::admin_author_list(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    if (authenticated_admin(req, session, db) == Session::invalid_user_id()) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    const std::vector authors{ db.author_list() };
    res.set_content(nlohmann::json(authors).dump(), "application/json");
}

inline void server::admin_add_author(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    if (authenticated_admin(req, session, db) == Session::invalid_user_id()) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    const std::string author{ su::trim(req.get_param_value("name")) };

    if (!validate_field(res, author)) {
        return;
    }

    if (db.author_exists(author)) {
        res.status = httplib::StatusCode::Conflict_409;
        res.set_content("Author already exists", "plain/text");
        return;
    }

    const std::optional author_id{ db.add_author(author) };
    if (!author_id) {
        res.status = httplib::StatusCode::InternalServerError_500;
        res.set_content("Fail to add the author", "plain/text");
        logging::error{ R"(Fail to add author "{}")", author };
        return;
    }

    res.set_content(nlohmann::json({ { "id", *author_id } }).dump(), "application/json");
}

inline void server::admin_delete_author(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    if (authenticated_admin(req, session, db) == Session::invalid_user_id()) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }
    const int author_id{ su::string_to_int(req.path_params.at("author_id")) };
    if (!db.delete_author(author_id)) {
        res.status = httplib::StatusCode::InternalServerError_500;
        logging::error{ R"(Fail to delete author "{}")", author_id };
        return;
    }

    res.status = httplib::StatusCode::OK_200;
}

inline void server::admin_tag_list(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    if (authenticated_admin(req, session, db) == Session::invalid_user_id()) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    const std::vector tags{ db.tag_list() };
    res.set_content(nlohmann::json(tags).dump(), "application/json");
}

inline void server::admin_add_tag(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    if (authenticated_admin(req, session, db) == Session::invalid_user_id()) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    const std::string tag{ su::trim(req.get_param_value("name")) };

    if (!validate_field(res, tag)) {
        return;
    }

    if (db.tag_exists(tag)) {
        res.status = httplib::StatusCode::Conflict_409;
        res.set_content("Tag already exists", "plain/text");
        return;
    }

    const std::optional tag_id{ db.add_tag(tag) };
    if (!tag_id) {
        res.status = httplib::StatusCode::InternalServerError_500;
        res.set_content("Fail to add the tag", "plain/text");
        logging::error{ R"(Fail to add tag "{}")", tag };
        return;
    }

    res.set_content(nlohmann::json({ { "id", *tag_id } }).dump(), "application/json");
}

inline void server::admin_delete_tag(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    if (authenticated_admin(req, session, db) == Session::invalid_user_id()) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    const int tag_id{ su::string_to_int(req.path_params.at("tag_id")) };
    if (!db.delete_tag(tag_id)) {
        res.status = httplib::StatusCode::InternalServerError_500;
        logging::error{ R"(Fail to delete tag "{}")", tag_id };
        return;
    }

    res.status = httplib::StatusCode::OK_200;
}

// Admin - Admins

inline void server::admin_admin_list(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    if (authenticated_admin(req, session, db) == Session::invalid_user_id()) {
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
            .is_logged_once = admin.password.has_value(),
            .is_deactivated = db.deactivated_user(admin.id)
        };
    });

    res.set_content(nlohmann::json(admin_admins).dump(), "application/json");
}

inline void server::admin_admin(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    if (authenticated_admin(req, session, db) == Session::invalid_user_id()) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    const int admin_id{ su::string_to_int(req.path_params.at("admin_id")) };
    const std::optional admin{ db.user(admin_id) };
    if (!admin) {
        res.status = httplib::StatusCode::NotFound_404;
        return;
    }

    const AdminAdminInfo admin_admin{
        .id = admin_id,
        .name = admin->name,
        .is_super_admin = db.is_super_admin(admin_id),
        .is_logged_once = admin->password.has_value(),
        .is_deactivated = db.deactivated_user(admin_id)
    };

    res.set_content(nlohmann::json(admin_admin).dump(), "application/json");
}

inline void server::admin_add_admin(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    if (authenticated_admin(req, session, db) == Session::invalid_user_id()) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    if (!req.has_param("username")) {
        res.status = httplib::StatusCode::BadRequest_400;
        res.set_content("Missing username field", "plain/text");
        return;
    }

    const std::string username{ su::trim(req.get_param_value("username")) };

    if (!validate_field(res, username)) {
        return;
    }

    if (db.user_exists(username)) {
        res.status = httplib::StatusCode::Conflict_409;
        res.set_content("Username already exists", "plain/text");
        return;
    }

    const std::string salt{ crypto::random_string() };
    const std::optional admin_id{ db.add_admin(username, salt) };
    if (!admin_id) {
        res.status = httplib::StatusCode::InternalServerError_500;
        res.set_content("Fail to add the admin", "plain/text");
        logging::error{ R"(Fail to add admin "{}")", username };
        return;
    }

    res.set_content(nlohmann::json({ { "id", *admin_id } }).dump(), "application/json");
}

inline void server::admin_update_admin(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    if (authenticated_admin(req, session, db) == Session::invalid_user_id()) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    if (!req.has_param("username")) {
        res.status = httplib::StatusCode::BadRequest_400;
        res.set_content("Missing username field", "plain/text");
        return;
    }

    const int user_id{ su::string_to_int(req.path_params.at("admin_id")) };
    const std::string username{ su::trim(req.get_param_value("username")) };

    if (!validate_field(res, username)) {
        return;
    }

    if (db.user_exists(user_id, username)) {
        res.status = httplib::StatusCode::Conflict_409;
        res.set_content("Username already exists", "plain/text");
        return;
    }

    if (!db.update_username(user_id, username)) {
        res.status = httplib::StatusCode::InternalServerError_500;
        res.set_content("Fail to update the user", "plain/text");
        logging::error{ R"(Fail to update user "{}")", user_id };
        return;
    }

    res.status = httplib::StatusCode::OK_200;
}

// Admin - Users

inline void server::admin_user_list(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    if (authenticated_admin(req, session, db) == Session::invalid_user_id()) {
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
            .videos = db.unique_user_video_list(user.id),
            .is_logged_once = user.password.has_value(),
            .is_deactivated = db.deactivated_user(user.id)
        };
    });

    res.set_content(nlohmann::json(admin_users).dump(), "application/json");
}

inline void server::admin_user(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    if (authenticated_admin(req, session, db) == Session::invalid_user_id()) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    const int user_id{ su::string_to_int(req.path_params.at("user_id")) };
    const std::optional user{ db.user(user_id) };
    if (!user) {
        res.status = httplib::StatusCode::NotFound_404;
        return;
    }

    const AdminUserInfo admin_user{
        .id = user_id,
        .name = user->name,
        .groups = db.user_group_list(user_id),
        .videos = db.unique_user_video_list(user_id),
        .is_logged_once = user->password.has_value(),
        .is_deactivated = db.deactivated_user(user_id)
    };

    res.set_content(nlohmann::json(admin_user).dump(), "application/json");
}

inline void server::admin_add_user(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    if (authenticated_admin(req, session, db) == Session::invalid_user_id()) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    if (!req.has_param("username")) {
        res.status = httplib::StatusCode::BadRequest_400;
        res.set_content("Missing username field", "plain/text");
        return;
    }

    const std::string username{ su::trim(req.get_param_value("username")) };

    if (!validate_field(res, username)) {
        return;
    }

    if (db.user_exists(username)) {
        res.status = httplib::StatusCode::Conflict_409;
        res.set_content("Username already exists", "plain/text");
        return;
    }

    const std::string salt{ crypto::random_string() };
    const std::vector<int> group_ids{ extract_ids(req.get_param_value("groupIds")) };
    const std::vector<int> video_ids{ extract_ids(req.get_param_value("videoIds")) };

    const std::optional user_id{
        db.add_user(username, salt)
            .and_then([&](int id) -> std::optional<int> {
                return db.add_user_groups(id, group_ids) ? std::optional(id) : std::nullopt;
            })
            .and_then([&](int id) -> std::optional<int> {
                return db.add_user_video_rights(id, video_ids) ? std::optional(id) : std::nullopt;
            })
    };

    if (!user_id) {
        res.status = httplib::StatusCode::InternalServerError_500;
        res.set_content("Fail to add the user", "plain/text");
        logging::error{ R"(Fail to add user "{}")", username };
        return;
    }

    res.set_content(nlohmann::json({ { "id", *user_id } }).dump(), "application/json");
}

inline void server::admin_update_user(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    if (authenticated_admin(req, session, db) == Session::invalid_user_id()) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    if (!req.has_param("username")) {
        res.status = httplib::StatusCode::BadRequest_400;
        res.set_content("Missing username field", "plain/text");
        return;
    }

    const int user_id{ su::string_to_int(req.path_params.at("user_id")) };
    const std::string username{ su::trim(req.get_param_value("username")) };

    if (!validate_field(res, username)) {
        return;
    }

    if (db.user_exists(user_id, username)) {
        res.status = httplib::StatusCode::Conflict_409;
        res.set_content("Username already exists", "plain/text");
        return;
    }

    const std::vector<int> group_ids{ extract_ids(req.get_param_value("groupIds")) };
    const std::vector<int> video_ids{ extract_ids(req.get_param_value("videoIds")) };

    const std::optional success{
        db.update_username(user_id, username)
            .and_then([&](int id) -> std::optional<int> {
                return db.update_user_groups(id, group_ids) ? std::optional(id) : std::nullopt;
            })
            .transform([&](int id) -> bool {
                return db.update_user_video_rights(id, video_ids);
            })
    };

    if (!success.value_or(false)) {
        res.status = httplib::StatusCode::InternalServerError_500;
        res.set_content("Fail to update the user", "plain/text");
        logging::error{ R"(Fail to update user "{}")", user_id };
        return;
    }

    res.status = httplib::StatusCode::OK_200;
}

inline void server::admin_deactivate_user(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    if (authenticated_admin(req, session, db) == Session::invalid_user_id()) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    const int user_id{ su::string_to_int(req.path_params.at("user_id")) };
    const bool deactivated{ su::string_to_bool(req.get_param_value("deactivated")) };

    if (!db.deactivate_user(user_id, deactivated)) {
        res.status = httplib::StatusCode::InternalServerError_500;
        logging::error{ R"(Fail to deactivate "{}" user "{}")", deactivated, user_id };
        return;
    }

    res.status = httplib::StatusCode::OK_200;
}

inline void server::admin_reset_user_password(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    if (authenticated_admin(req, session, db) == Session::invalid_user_id()) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    const int user_id{ su::string_to_int(req.path_params.at("user_id")) };
    if (!db.clear_password(user_id)) {
        res.status = httplib::StatusCode::InternalServerError_500;
        logging::error{ R"(Fail to reset user password "{}")", user_id };
        return;
    }

    res.status = httplib::StatusCode::OK_200;
}

inline void server::admin_delete_user(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    if (authenticated_admin(req, session, db) == Session::invalid_user_id()) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    const int user_id{ su::string_to_int(req.path_params.at("user_id")) };
    if (!db.delete_user(user_id)) {
        res.status = httplib::StatusCode::InternalServerError_500;
        logging::error{ R"(Fail to delete user "{}")", user_id };
        return;
    }

    res.status = httplib::StatusCode::OK_200;
}

// Admin - Groups

inline void server::admin_group_list(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    if (authenticated_admin(req, session, db) == Session::invalid_user_id()) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    std::vector<Group> groups{ db.group_list() };
    std::vector<AdminGroupInfo> admin_groups(groups.size());
    std::ranges::transform(groups, admin_groups.begin(), [&db](const Group& group) {
        return AdminGroupInfo{
            .id = group.id,
            .name = group.name,
            .users = db.group_user_list(group.id),
            .videos = db.group_video_list(group.id)
        };
    });

    res.set_content(nlohmann::json(admin_groups).dump(), "application/json");
}

inline void server::admin_group(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    if (authenticated_admin(req, session, db) == Session::invalid_user_id()) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    const int group_id{ su::string_to_int(req.path_params.at("group_id")) };
    const std::optional group{ db.group(group_id) };
    if (!group) {
        res.status = httplib::StatusCode::NotFound_404;
        return;
    }

    const AdminGroupInfo admin_group{
        .id = group_id,
        .name = group->name,
        .users = db.group_user_list(group_id),
        .videos = db.group_video_list(group_id)
    };

    res.set_content(nlohmann::json(admin_group).dump(), "application/json");
}

inline void server::admin_add_group(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    if (authenticated_admin(req, session, db) == Session::invalid_user_id()) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    if (!req.has_param("name")) {
        res.status = httplib::StatusCode::BadRequest_400;
        res.set_content("Missing name field", "plain/text");
        return;
    }

    const std::string group_name{ su::trim(req.get_param_value("name")) };
    const std::vector<int> user_ids{ extract_ids(req.get_param_value("userIds")) };
    const std::vector<int> video_ids{ extract_ids(req.get_param_value("videoIds")) };

    if (!validate_field(res, group_name)) {
        return;
    }

    if (db.group_exists(group_name)) {
        res.status = httplib::StatusCode::Conflict_409;
        res.set_content("Group already exists", "plain/text");
        return;
    }

    const std::optional group_id{
        db.add_group(group_name)
            .and_then([&](int id) -> std::optional<int> {
                return db.add_group_users(id, user_ids) ? std::optional(id) : std::nullopt;
            })
            .and_then([&](int id) -> std::optional<int> {
                return db.add_group_video_rights(id, video_ids) ? std::optional(id) : std::nullopt;
            })
    };

    if (!group_id) {
        res.status = httplib::StatusCode::InternalServerError_500;
        res.set_content("Fail to add the group", "plain/text");
        logging::error{ R"(Fail to add group "{}")", group_name };
        return;
    }

    res.set_content(nlohmann::json({ { "id", *group_id } }).dump(), "application/json");
}

inline void server::admin_update_group(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    if (authenticated_admin(req, session, db) == Session::invalid_user_id()) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    if (!req.has_param("name")) {
        res.status = httplib::StatusCode::BadRequest_400;
        res.set_content("Missing group name field", "plain/text");
        return;
    }

    const int group_id{ su::string_to_int(req.path_params.at("group_id")) };
    const std::string group_name{ su::trim(req.get_param_value("name")) };
    const std::vector<int> user_ids{ extract_ids(req.get_param_value("userIds")) };
    const std::vector<int> video_ids{ extract_ids(req.get_param_value("videoIds")) };

    if (!validate_field(res, group_name)) {
        return;
    }

    if (db.group_exists(group_id, group_name)) {
        res.status = httplib::StatusCode::Conflict_409;
        res.set_content("Group already exists", "plain/text");
        return;
    }

    const std::optional success{
        db.update_group_name(group_id, group_name)
            .and_then([&](int id) -> std::optional<int> {
                return db.update_group_users(id, user_ids) ? std::optional(id) : std::nullopt;
            })
            .transform([&](int id) -> bool {
                return db.update_group_video_rights(id, video_ids);
            })
    };

    if (!success.value_or(false)) {
        res.status = httplib::StatusCode::InternalServerError_500;
        res.set_content("Fail to update the group", "plain/text");
        logging::error{ R"(Fail to update group "{}")", group_id };
        return;
    }

    res.status = httplib::StatusCode::OK_200;
}

inline void server::admin_delete_group(const httplib::Request& req, httplib::Response& res, const Session& session, const Database& db)
{
    if (authenticated_admin(req, session, db) == Session::invalid_user_id()) {
        res.status = httplib::StatusCode::Unauthorized_401;
        return;
    }

    const int group_id{ su::string_to_int(req.path_params.at("group_id")) };
    if (!db.delete_group(group_id)) {
        res.status = httplib::StatusCode::InternalServerError_500;
        logging::error{ R"(Fail to delete group "{}")", group_id };
        return;
    }

    res.status = httplib::StatusCode::OK_200;
}
