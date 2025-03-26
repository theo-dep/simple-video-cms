#include "server.h"

#include "crypto.h"
#include "database.h"
#include "filesystem.h"
#include "logcontroller.h"
#include "logging.h"
#include "search.h"
#include "servercommon.h"
#include "stringutils.h"
#include "video.h"

#include <httplib.h>

#include <stacktrace>

namespace server
{
    bool create_super_admin(const Database& db);

    void set_logger(httplib::Server& server, LogController& video_log_controller);
    void set_exception_handler(httplib::Server& server);

    void template_page(const httplib::Request& req, httplib::Response& res);
    void static_file(const httplib::Request& req, httplib::Response& res);

    void admin_video_list(const httplib::Request& req, httplib::Response& res, const Database& db);
    void user_video_list(const httplib::Request& req, httplib::Response& res, const Database& db);

    void video_title(const httplib::Request& req, httplib::Response& res, const Database& db);
    void video_views(const httplib::Request& req, httplib::Response& res, const Database& db);

    void user_count(const httplib::Request& req, httplib::Response& res, const Database& db);
    void video_count(const httplib::Request& req, httplib::Response& res, const Database& db);
    void view_count(const httplib::Request& req, httplib::Response& res, const Database& db);

    void is_admin(const httplib::Request& req, httplib::Response& res, const Database& db);
    void is_super_admin(const httplib::Request& req, httplib::Response& res, const Database& db);
    void is_user(const httplib::Request& req, httplib::Response& res, const Database& db);

    void add_admin(const httplib::Request& req, httplib::Response& res, const Database& db);
    void add_user(const httplib::Request& req, httplib::Response& res, const Database& db);
    void update_user(const httplib::Request& req, httplib::Response& res, const Database& db);
    void delete_user(const httplib::Request& req, httplib::Response& res, const Database& db);
    void is_valid_user(const httplib::Request& req, httplib::Response& res, const Database& db);

    void user_name(const httplib::Request& req, httplib::Response& res, const Database& db);
    void user_id(const httplib::Request& req, httplib::Response& res, const Database& db);

    void user_list(const httplib::Request& req, httplib::Response& res, const Database& db);
    void admin_list(const httplib::Request& req, httplib::Response& res, const Database& db);

    void add_video(const httplib::Request& req, httplib::Response& res, const Database& db);
    void update_video(const httplib::Request& req, httplib::Response& res, const Database& db);
    void delete_video(const httplib::Request& req, httplib::Response& res, const Database& db);
    void increment_video_views(const httplib::Request& req, httplib::Response& res, const Database& db);
    void video(const httplib::Request& req, httplib::Response& res, const Database& db);
    void video_size(const httplib::Request& req, httplib::Response& res, const Database& db);
    void thumbnail(const httplib::Request& req, httplib::Response& res, const Database& db);
    void has_video_right(const httplib::Request& req, httplib::Response& res, const Database& db);

    void video_right_list(const httplib::Request& req, httplib::Response& res, const Database& db);
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

    LogController video_log_controller("/video/");
    set_logger(server, video_log_controller);
    set_exception_handler(server);

    server
        .Get("/html/:html", sc::serve(template_page))
        .Get(sc::static_regexp_path(), sc::serve(static_file))

        .Get("/admin-video-list", sc::serve(admin_video_list, std::cref(db)))
        .Get("/user-video-list", sc::serve(user_video_list, std::cref(db)))

        .Get("/title/:video_id", sc::serve(video_title, std::cref(db)))
        .Get("/views/:video_id", sc::serve(video_views, std::cref(db)))

        .Get("/user-count", sc::serve(user_count, std::cref(db)))
        .Get("/video-count", sc::serve(video_count, std::cref(db)))
        .Get("/view-count", sc::serve(view_count, std::cref(db)))

        .Get("/is-admin", sc::serve(is_admin, std::cref(db)))
        .Get("/is-super-admin", sc::serve(is_super_admin, std::cref(db)))
        .Get("/is-user", sc::serve(is_user, std::cref(db)))

        .Get("/user-name", sc::serve(user_name, std::cref(db)))
        .Get("/user-id", sc::serve(user_id, std::cref(db)))

        .Post("/add-admin", sc::serve(add_admin, std::cref(db)))
        .Post("/add-user", sc::serve(add_user, std::cref(db)))
        .Post("/update-user", sc::serve(update_user, std::cref(db)))
        .Post("/delete-user", sc::serve(delete_user, std::cref(db)))
        .Post("/is-valid-user", sc::serve(is_valid_user, std::cref(db)))

        .Get("/user-list", sc::serve(user_list, std::cref(db)))
        .Get("/admin-list", sc::serve(admin_list, std::cref(db)))

        .Post("/add-video", sc::serve(add_video, std::cref(db)))
        .Post("/update-video/:video_id", sc::serve(update_video, std::cref(db)))
        .Post("/delete-video/:video_id", sc::serve(delete_video, std::cref(db)))
        .Post("/increment-video-views/:video_id", sc::serve(increment_video_views, std::cref(db)))
        .Get("/video/:video_id", sc::serve(video, std::cref(db)))
        .Get("/video-size/:video_id", sc::serve(video_size, std::cref(db)))
        .Get("/thumbnail/:video_id", sc::serve(thumbnail, std::cref(db)))
        .Get("/has-video-right/:video_id", sc::serve(has_video_right, std::cref(db)))

        .Get("/video-right-list/:video_id", sc::serve(video_right_list, std::cref(db)));

    const std::string host{ sc::get_env("BACK_HOST", "0.0.0.0") };
    const int port{ su::string_to_int(sc::get_env("BACK_PORT", "5000")) };
    logging::info{ "Serving HTTP on {0} port {1} ...", host, port };
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

    const std::string password{ crypto::sha512(sc::get_env("SUPER_ADMIN_PASSWORD", "admin")) };
    const std::string salt{ crypto::random_string() };
    logging::debug{ "Salt: {}", salt };
    if (!db.add_super_admin(username, crypto::password(password, salt), salt)) {
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

inline void server::template_page(const httplib::Request& req, httplib::Response& res)
{
    const std::string html{ req.path_params.at("html") };
    const std::filesystem::path html_path{ std::filesystem::current_path() / "templates" / html };
    res.set_file_content(html_path.string(), "text/html");
}

inline void server::static_file(const httplib::Request& req, httplib::Response& res)
{
    const std::string file{ req.matches[1] };
    const std::filesystem::path file_path{ std::filesystem::current_path() / "static" / file };
    res.set_file_content(file_path.string()); // let content_type empty, httplib will find the right type
    // if supported in https://github.com/yhirose/cpp-httplib?tab=readme-ov-file#static-file-server
}

namespace server
{
    std::vector<std::string> transform(const std::vector<int>& list);
    std::vector<int> transform(const std::vector<std::string>& list);
    std::vector<int> extract(const Database& db, const std::string& search, const std::vector<int>& ids);
}

inline std::vector<std::string> server::transform(const std::vector<int>& list)
{
    std::vector<std::string> str_list(list.size());
    std::ranges::transform(list, str_list.begin(), su::int_to_string);
    return str_list;
}

inline std::vector<int> server::transform(const std::vector<std::string>& list)
{
    std::vector<int> int_list(list.size());
    std::ranges::transform(list, int_list.begin(), su::string_to_int);
    return int_list;
}

inline std::vector<int> server::extract(const Database& db, const std::string& search, const std::vector<int>& ids)
{
    std::unordered_map<std::string, int> title_ids;
    title_ids.reserve(ids.size());
    std::ranges::transform(ids, std::inserter(title_ids, title_ids.end()),
                           [&db](int id) -> decltype(title_ids)::value_type {
                               return std::make_pair(db.video_title(id), id);
                           });
    return search::extract(search, title_ids);
}

inline void server::admin_video_list(const httplib::Request& req, httplib::Response& res, const Database& db)
{
    std::vector ids{ db.admin_video_list() };
    if (req.has_param("search")) {
        const std::string search{ req.get_param_value("search") };
        ids = extract(db, search, ids);
    }

    const std::vector str_ids{ transform(ids) };
    res.set_content(su::join(str_ids), "plain/text");
}

inline void server::user_video_list(const httplib::Request& req, httplib::Response& res, const Database& db)
{
    std::vector ids{ db.no_user_video_list() };
    if (req.has_header("user_id")) {
        const int user_id{ su::string_to_int(req.get_header_value("user_id")) };
        const std::vector user_ids{ db.user_video_list(user_id) };
#ifdef __cpp_lib_containers_ranges
        ids.append_range(user_ids);
#else
        ids.insert(ids.end(), user_ids.cbegin(), user_ids.cend());
#endif
    }

    if (req.has_param("search")) {
        const std::string search{ req.get_param_value("search") };
        ids = extract(db, search, ids);
    }

    const std::vector str_ids{ transform(ids) };
    res.set_content(su::join(str_ids), "plain/text");
}

inline void server::video_title(const httplib::Request& req, httplib::Response& res, const Database& db)
{
    const int video_id{ su::string_to_int(req.path_params.at("video_id")) };
    const std::string video_title{ db.video_title(video_id) };
    res.set_content(video_title, "plain/text");
}

inline void server::video_views(const httplib::Request& req, httplib::Response& res, const Database& db)
{
    const int video_id{ su::string_to_int(req.path_params.at("video_id")) };
    const int video_views{ db.video_views(video_id) };
    res.set_content(su::int_to_string(video_views), "plain/text");
}

inline void server::user_count(const httplib::Request& /*req*/, httplib::Response& res, const Database& db)
{
    const int count{ db.user_count() };
    res.set_content(su::int_to_string(count), "plain/text");
}

inline void server::video_count(const httplib::Request& /*req*/, httplib::Response& res, const Database& db)
{
    const int count{ db.video_count() };
    res.set_content(su::int_to_string(count), "plain/text");
}

inline void server::view_count(const httplib::Request& /*req*/, httplib::Response& res, const Database& db)
{
    const int count{ db.view_count() };
    res.set_content(su::int_to_string(count), "plain/text");
}

inline void server::is_admin(const httplib::Request& req, httplib::Response& res, const Database& db)
{
    if (!req.has_header("user_id")) {
        logging::error{ "Missing header data" };
        res.status = httplib::StatusCode::InternalServerError_500;
        return;
    }

    const int user_id{ su::string_to_int(req.get_header_value("user_id")) };
    const bool is_admin{ db.is_admin(user_id) };
    res.set_content(su::bool_to_string(is_admin), "plain/text");
}

inline void server::is_super_admin(const httplib::Request& req, httplib::Response& res, const Database& db)
{
    if (!req.has_header("user_id")) {
        logging::error{ "Missing header data" };
        res.status = httplib::StatusCode::InternalServerError_500;
        return;
    }

    const int user_id{ su::string_to_int(req.get_header_value("user_id")) };
    const bool is_super_admin{ db.is_super_admin(user_id) };
    res.set_content(su::bool_to_string(is_super_admin), "plain/text");
}

inline void server::is_user(const httplib::Request& req, httplib::Response& res, const Database& db)
{
    if (!req.has_header("user_id")) {
        logging::error{ "Missing header data" };
        res.status = httplib::StatusCode::InternalServerError_500;
        return;
    }

    const int user_id{ su::string_to_int(req.get_header_value("user_id")) };
    const bool is_user{ db.is_user(user_id) };
    res.set_content(su::bool_to_string(is_user), "plain/text");
}

inline void server::user_name(const httplib::Request& req, httplib::Response& res, const Database& db)
{
    if (!req.has_header("user_id")) {
        logging::error{ "Missing header data" };
        res.status = httplib::StatusCode::InternalServerError_500;
        return;
    }

    const int user_id{ su::string_to_int(req.get_header_value("user_id")) };
    const std::string username{ db.user_name(user_id) };
    res.set_content(username, "plain/text");
}

inline void server::user_id(const httplib::Request& req, httplib::Response& res, const Database& db)
{
    if (!req.has_header("username")) {
        logging::error{ "Missing header data" };
        res.status = httplib::StatusCode::InternalServerError_500;
        return;
    }

    const std::string username{ req.get_header_value("username") };
    const int user_id{ db.user_id(username) };
    res.set_content(su::int_to_string(user_id), "plain/text");
}

inline void server::add_admin(const httplib::Request& req, httplib::Response& res, const Database& db)
{
    if (!req.has_file("password") || !req.has_file("username")) {
        logging::error{ "Missing multipart form data" };
        res.status = httplib::StatusCode::InternalServerError_500;
        return;
    }

    const std::string username{ req.get_file_value("username").content };
    const std::string salt{ crypto::random_string() };
    logging::debug{ "Salt: {}", salt };
    const std::string password{ crypto::password(req.get_file_value("password").content, salt) };
    const std::optional admin_id{ db.add_admin(username, password, salt) };
    if (!admin_id.has_value()) {
        logging::error{ R"(Fail to add admin "{}")", username };
        res.status = httplib::StatusCode::InternalServerError_500;
        return;
    }

    res.set_content(su::int_to_string(admin_id.value()), "plan/text");
}

inline void server::add_user(const httplib::Request& req, httplib::Response& res, const Database& db)
{
    if (!req.has_file("password") || !req.has_file("username")) {
        logging::error{ "Missing multipart form data" };
        res.status = httplib::StatusCode::InternalServerError_500;
        return;
    }

    const std::string username{ req.get_file_value("username").content };
    const std::string salt{ crypto::random_string() };
    logging::debug{ "Salt: {}", salt };
    const std::string password{ crypto::password(req.get_file_value("password").content, salt) };
    const std::optional user_id{ db.add_user(username, password, salt) };
    if (!user_id.has_value()) {
        logging::error{ R"(Fail to add user "{}")", username };
        res.status = httplib::StatusCode::InternalServerError_500;
        return;
    }

    res.set_content(su::int_to_string(user_id.value()), "plan/text");
}

inline void server::update_user(const httplib::Request& req, httplib::Response& res, const Database& db)
{
    if (!req.has_file("password") || !req.has_file("username") || !req.has_file("user_id")) {
        logging::error{ "Missing multipart form data" };
        res.status = httplib::StatusCode::InternalServerError_500;
        return;
    }

    const int user_id{ su::string_to_int(req.get_file_value("user_id").content) };
    const std::string username{ req.get_file_value("username").content };
    const std::string salt{ db.user_salt(user_id) };
    const std::string password{ crypto::password(req.get_file_value("password").content, salt) };

    const std::optional success_user_id{
        db.update_user_name(user_id, username)
            .and_then([&](int user_id) -> std::optional<int> {
                return db.update_user_password(user_id, password);
            })
    };

    if (!success_user_id.has_value()) {
        logging::error{ R"(Fail to update user "{}")", user_id };
        return;
    }
}

inline void server::delete_user(const httplib::Request& req, httplib::Response& res, const Database& db)
{
    if (!req.has_file("user_id")) {
        logging::error{ "Missing multipart form data" };
        res.status = httplib::StatusCode::InternalServerError_500;
        return;
    }

    const int user_id{ su::string_to_int(req.get_file_value("user_id").content) };
    if (!db.delete_user(user_id)) {
        logging::error{ R"(Fail to delete user "{}")", user_id };
        return;
    }
}

inline void server::is_valid_user(const httplib::Request& req, httplib::Response& res, const Database& db)
{
    if (!req.has_file("password") || !req.has_file("user_id")) {
        logging::error{ "Missing multipart form data" };
        res.status = httplib::StatusCode::InternalServerError_500;
        return;
    }

    const int user_id{ su::string_to_int(req.get_file_value("user_id").content) };
    const std::string salt{ db.user_salt(user_id) };
    const std::string password{ crypto::password(req.get_file_value("password").content, salt) };

    const std::string database_password{ db.user_password(user_id) };
    res.set_content(su::bool_to_string(password == database_password), "plain/text");
}

inline void server::user_list(const httplib::Request& /*req*/, httplib::Response& res, const Database& db)
{
    const std::vector user_ids{ transform(db.user_list()) };
    res.set_content(su::join(user_ids), "plain/text");
}

inline void server::admin_list(const httplib::Request& /*req*/, httplib::Response& res, const Database& db)
{
    const std::vector user_ids{ transform(db.admin_list()) };
    res.set_content(su::join(user_ids), "plain/text");
}

inline void server::add_video(const httplib::Request& req, httplib::Response& res, const Database& db)
{
    if (!req.has_file("title") || !req.has_file("video") || !req.has_file("user_ids")) {
        logging::error{ "Missing multipart form data" };
        res.status = httplib::StatusCode::InternalServerError_500;
        return;
    }

    const std::string video_title{ req.get_file_value("title").content };
    const std::string video_content{ req.get_file_value("video").content };
    const std::vector allowed_user_ids{ su::split(req.get_file_value("user_ids").content) };
    const std::string thumbnail_content{ video::thumbnail(video_content) };

    const std::optional video_id{
        db.add_video(video_title, video_content)
            .and_then([&](int video_id) -> std::optional<int> {
                return db.add_video_thumbnail(video_id, thumbnail_content);
            })
            .and_then([&](int video_id) -> std::optional<int> {
                return db.add_video_rights(video_id, transform(allowed_user_ids)) ? std::optional(video_id) : std::nullopt;
            })
    };

    if (!video_id.has_value()) {
        logging::error{ R"(Fail to add video "{}")", video_title };
        res.status = httplib::StatusCode::InternalServerError_500;
        return;
    }

    res.set_content(su::int_to_string(video_id.value()), "plain/text");
}

inline void server::update_video(const httplib::Request& req, httplib::Response& res, const Database& db)
{
    if (!req.has_file("title") || !req.has_file("user_ids")) {
        logging::error{ "Missing multipart form data" };
        res.status = httplib::StatusCode::InternalServerError_500;
        return;
    }

    const int video_id{ su::string_to_int(req.path_params.at("video_id")) };
    const std::string video_title{ req.get_file_value("title").content };
    const std::vector allowed_user_ids{ su::split(req.get_file_value("user_ids").content) };

    const std::optional success{
        db.update_video_title(video_id, video_title)
            .transform([&](int video_id) -> bool {
                return db.update_video_rights(video_id, transform(allowed_user_ids));
            })
    };

    if (!success.value_or(false)) {
        logging::error{ R"(Fail to update video "{}")", video_id };
        return;
    }
}

inline void server::delete_video(const httplib::Request& req, httplib::Response& /*res*/, const Database& db)
{
    const int video_id{ su::string_to_int(req.path_params.at("video_id")) };
    if (!db.delete_video(video_id)) {
        logging::error{ R"(Fail to delete video "{}")", video_id };
        return;
    }
}

inline void server::increment_video_views(const httplib::Request& req, httplib::Response& /*res*/, const Database& db)
{
    const int video_id{ su::string_to_int(req.path_params.at("video_id")) };
    if (!db.increment_video_views(video_id)) {
        logging::error{ R"(Fail to increment video views "{}")", video_id };
        return;
    }
}

inline void server::video(const httplib::Request& req, httplib::Response& res, const Database& db)
{
    const int video_id{ su::string_to_int(req.path_params.at("video_id")) };

    const std::size_t offset{
        req.has_header("Offset")
            ? static_cast<std::size_t>(su::string_to_int(req.get_header_value("Offset")))
            : 0U
    };

    const std::size_t length{
        req.has_header("Length")
            ? static_cast<std::size_t>(su::string_to_int(req.get_header_value("Length")))
            : static_cast<std::size_t>(db.video_size(video_id))
    };

    const std::string video{ db.video(video_id, offset, length) };
    res.set_content(video, "video/mp4");
}

inline void server::video_size(const httplib::Request& req, httplib::Response& res, const Database& db)
{
    const int video_id{ su::string_to_int(req.path_params.at("video_id")) };
    const int video_size{ db.video_size(video_id) };
    res.set_content(su::int_to_string(video_size), "plain/text");
}

inline void server::thumbnail(const httplib::Request& req, httplib::Response& res, const Database& db)
{
    const int video_id{ su::string_to_int(req.path_params.at("video_id")) };
    const std::string thumbnail{ db.thumbnail(video_id) };
    res.set_content(thumbnail, "image/png");
}

inline void server::has_video_right(const httplib::Request& req, httplib::Response& res, const Database& db)
{
    const int video_id{ su::string_to_int(req.path_params.at("video_id")) };

    bool has_video_right{ db.has_video_right(video_id) };
    if (!has_video_right && req.has_header("user_id")) {
        const int user_id{ su::string_to_int(req.get_header_value("user_id")) };
        // check if user is admin
        has_video_right = db.is_admin(user_id) || db.has_video_right(video_id, user_id);
    }

    res.set_content(su::bool_to_string(has_video_right), "plain/text");
}

inline void server::video_right_list(const httplib::Request& req, httplib::Response& res, const Database& db)
{
    const int video_id{ su::string_to_int(req.path_params.at("video_id")) };

    const std::vector rights{ transform(db.video_right_list(video_id)) };
    res.set_content(su::join(rights), "plain/text");
}
