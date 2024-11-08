#include "server.h"

#include "crypto.h"
#include "database.h"
#include "logging.h"
#include "servercommon.h"
#include "stringutils.h"

#include <httplib.h>

#include <filesystem>
#include <fstream>

namespace server
{
    void create_super_admin(const Database& db) noexcept;

    void template_page(const httplib::Request& req, httplib::Response& res) noexcept;
    void static_file(const httplib::Request& req, httplib::Response& res) noexcept;

    void video_list(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept;
    void no_right_video_list(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept;

    void video_title(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept;
    void video_views(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept;

    void user_count(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept;
    void video_count(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept;
    void view_count(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept;

    void is_admin(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept;
    void is_super_admin(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept;
    void is_user(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept;

    void add_admin(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept;
    void add_user(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept;
    void update_admin(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept;
    void update_user(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept;
    void delete_admin(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept;
    void delete_user(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept;
    void is_valid_user(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept;

    void user_name(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept;
    void user_id(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept;

    void user_list(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept;
    void admin_list(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept;

    void add_video(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept;
    void update_video(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept;
    void delete_video(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept;
    void increment_video_views(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept;
    void video(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept;
    void has_video_right(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept;

    void video_right_list(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept;
}

int server::start() noexcept
{
    const std::filesystem::path database_file{ std::filesystem::current_path() / "data" / "video.db" };

    bool is_database_ok{ false };
    Database db(database_file, is_database_ok);
    if (!is_database_ok) {
        logging::error{ "Fail to create the database" };
        return EXIT_FAILURE;
    }

    if (!db.create_tables()) {
        logging::error{ "Fail to create database tables" };
        return EXIT_FAILURE;
    }

    create_super_admin(db);

    httplib::Server server;
    server.set_logger([](const httplib::Request& req, const httplib::Response& res) {
        logging::raw_log(sc::log(req, res));
    });

    server
        .Get("/html/:html", sc::serve(template_page))
        .Get(sc::static_regexp_path(), sc::serve(static_file))

        .Get("/video-list", sc::serve(video_list, std::cref(db)))
        .Get("/no-right-video-list", sc::serve(no_right_video_list, std::cref(db)))

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
        .Post("/update-admin", sc::serve(update_admin, std::cref(db)))
        .Post("/update-user", sc::serve(update_user, std::cref(db)))
        .Post("/delete-admin", sc::serve(delete_admin, std::cref(db)))
        .Post("/delete-user", sc::serve(delete_user, std::cref(db)))
        .Post("/is-valid-user", sc::serve(is_valid_user, std::cref(db)))

        .Get("/user-list", sc::serve(user_list, std::cref(db)))
        .Get("/admin-list", sc::serve(admin_list, std::cref(db)))

        .Post("/add-video", sc::serve(add_video, std::cref(db)))
        .Post("/update-video/:video_id", sc::serve(update_video, std::cref(db)))
        .Post("/delete-video/:video_id", sc::serve(delete_video, std::cref(db)))
        .Post("/increment-video-views/:video_id", sc::serve(increment_video_views, std::cref(db)))
        .Get("/video/:video_id", sc::serve(video, std::cref(db)))
        .Get("/has-video-right/:video_id", sc::serve(has_video_right, std::cref(db)))

        .Get("/video-right-list/:video_id", sc::serve(video_right_list, std::cref(db)));

    constexpr const char* host{ "0.0.0.0" };
    constexpr int port{ 5000 };
    logging::info{ "Serving HTTP on {0} port {1} ...", host, port };
    return (server.listen(host, port) ? EXIT_SUCCESS : EXIT_FAILURE);
}

inline void server::create_super_admin(const Database& db) noexcept
{
    const std::string username{ sc::get_env("MYSQL_ADMIN_USERNAME", "admin") };
    const std::int64_t user_id{ db.user_id(username) };
    if (db.is_admin(user_id)) {
        // already created
        return;
    }

    const std::string password{ crypto::sha512(sc::get_env("MYSQL_ADMIN_PASSWORD", "admin")) };
    const std::string salt{ crypto::random_string() };
    logging::debug{ "Salt: {}", salt };
    if (!db.add_super_admin(username, crypto::password(password, salt), salt)) {
        logging::error{ "Fail to add super admin \"{}\"", username };
        return;
    }
}

inline void server::template_page(const httplib::Request& req, httplib::Response& res) noexcept
{
    const std::string html{ req.path_params.at("html") };
    const std::filesystem::path html_path{ std::filesystem::current_path() / "templates" / html };
    res.set_file_content(html_path.string(), "text/html");
}

inline void server::static_file(const httplib::Request& req, httplib::Response& res) noexcept
{
    const std::string file{ req.matches[1] };
    const std::filesystem::path file_path{ std::filesystem::current_path() / "static" / file };
    res.set_file_content(file_path.string()); // let content_type empty, httplib will find the right type
    // if supported in https://github.com/yhirose/cpp-httplib?tab=readme-ov-file#static-file-server
}

namespace server
{
    std::vector<std::string> transform(const std::vector<std::int64_t>& list) noexcept;
    std::vector<std::int64_t> transform(const std::vector<std::string>& list) noexcept;
}

inline std::vector<std::string> server::transform(const std::vector<std::int64_t>& list) noexcept
{
    std::vector<std::string> str_list(list.size());
    std::ranges::transform(list, str_list.begin(), su::int_to_string);
    return str_list;
}

inline std::vector<std::int64_t> server::transform(const std::vector<std::string>& list) noexcept
{
    std::vector<std::int64_t> int_list(list.size());
    std::ranges::transform(list, int_list.begin(), su::string_to_int);
    return int_list;
}

inline void server::video_list(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept
{
    std::vector<std::int64_t> varchar_ids;
    if (req.has_header("user_id")) {
        const std::int64_t user_id{ su::string_to_int(req.get_header_value("user_id")) };
        varchar_ids = db.video_list(user_id);
    } else {
        varchar_ids = db.video_list();
    }

    const std::vector ids{ transform(varchar_ids) };
    res.set_content(su::join(ids), "plain/text");
}

inline void server::no_right_video_list(const httplib::Request& /*req*/, httplib::Response& res, const Database& db) noexcept
{
    const std::vector ids{ transform(db.no_right_video_list()) };
    res.set_content(su::join(ids), "plain/text");
}

inline void server::video_title(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept
{
    const std::int64_t video_id{ su::string_to_int(req.path_params.at("video_id")) };
    const std::string video_title{ db.video_title(video_id) };
    res.set_content(video_title, "plain/text");
}

inline void server::video_views(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept
{
    const std::int64_t video_id{ su::string_to_int(req.path_params.at("video_id")) };
    const std::int64_t video_views{ db.video_views(video_id) };
    res.set_content(su::int_to_string(video_views), "plain/text");
}

inline void server::user_count(const httplib::Request& /*req*/, httplib::Response& res, const Database& db) noexcept
{
    const std::int64_t count{ db.user_count() };
    res.set_content(su::int_to_string(count), "plain/text");
}

inline void server::video_count(const httplib::Request& /*req*/, httplib::Response& res, const Database& db) noexcept
{
    const std::int64_t count{ db.video_count() };
    res.set_content(su::int_to_string(count), "plain/text");
}

inline void server::view_count(const httplib::Request& /*req*/, httplib::Response& res, const Database& db) noexcept
{
    const std::int64_t count{ db.view_count() };
    res.set_content(su::int_to_string(count), "plain/text");
}

inline void server::is_admin(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept
{
    if (!req.has_header("user_id")) {
        logging::error{ "Missing header data" };
        res.status = httplib::StatusCode::InternalServerError_500;
        return;
    }

    const std::int64_t user_id{ su::string_to_int(req.get_header_value("user_id")) };
    const bool is_admin{ db.is_admin(user_id) };
    res.set_content(su::bool_to_string(is_admin), "plain/text");
}

inline void server::is_super_admin(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept
{
    if (!req.has_header("user_id")) {
        logging::error{ "Missing header data" };
        res.status = httplib::StatusCode::InternalServerError_500;
        return;
    }

    const std::int64_t user_id{ su::string_to_int(req.get_header_value("user_id")) };
    const bool is_super_admin{ db.is_super_admin(user_id) };
    res.set_content(su::bool_to_string(is_super_admin), "plain/text");
}

inline void server::is_user(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept
{
    if (!req.has_header("user_id")) {
        logging::error{ "Missing header data" };
        res.status = httplib::StatusCode::InternalServerError_500;
        return;
    }

    const std::int64_t user_id{ su::string_to_int(req.get_header_value("user_id")) };
    const bool is_user{ db.is_user(user_id) };
    res.set_content(su::bool_to_string(is_user), "plain/text");
}

inline void server::user_name(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept
{
    if (!req.has_header("user_id")) {
        logging::error{ "Missing header data" };
        res.status = httplib::StatusCode::InternalServerError_500;
        return;
    }

    const std::int64_t user_id{ su::string_to_int(req.get_header_value("user_id")) };
    const std::string username{ db.user_name(user_id) };
    res.set_content(username, "plain/text");
}

inline void server::user_id(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept
{
    if (!req.has_header("username")) {
        logging::error{ "Missing header data" };
        res.status = httplib::StatusCode::InternalServerError_500;
        return;
    }

    const std::string username{ req.get_header_value("username") };
    const std::int64_t user_id{ db.user_id(username) };
    res.set_content(su::int_to_string(user_id), "plain/text");
}

inline void server::add_admin(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept
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
    if (!db.add_admin(username, password, salt)) {
        logging::error{ "Fail to add admin \"{}\"", username };
        return;
    }
}

inline void server::add_user(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept
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
    if (!db.add_user(username, password, salt)) {
        logging::error{ "Fail to add user \"{}\"", username };
        return;
    }
}

inline void server::update_admin(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept
{
    if (!req.has_file("password") || !req.has_file("user_id")) {
        logging::error{ "Missing multipart form data" };
        res.status = httplib::StatusCode::InternalServerError_500;
        return;
    }

    const std::int64_t user_id{ su::string_to_int(req.get_file_value("user_id").content) };
    if (db.is_super_admin(user_id)) {
        logging::error{ "Trying to update a super admin \"{}\"", user_id };
        res.status = httplib::StatusCode::Forbidden_403;
        return;
    }

    const std::string salt{ db.user_salt(user_id) };
    const std::string password{ crypto::password(req.get_file_value("password").content, salt) };
    if (!db.update_admin(user_id, password)) {
        logging::error{ "Fail to update admin \"{}\"", user_id };
        return;
    }
}

inline void server::update_user(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept
{
    if (!req.has_file("password") || !req.has_file("user_id")) {
        logging::error{ "Missing multipart form data" };
        res.status = httplib::StatusCode::InternalServerError_500;
        return;
    }

    const std::int64_t user_id{ su::string_to_int(req.get_file_value("user_id").content) };
    const std::string salt{ db.user_salt(user_id) };
    const std::string password{ crypto::password(req.get_file_value("password").content, salt) };
    if (!db.update_user(user_id, password)) {
        logging::error{ "Fail to update user \"{}\"", user_id };
        return;
    }
}

inline void server::delete_admin(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept
{
    if (!req.has_file("user_id")) {
        logging::error{ "Missing multipart form data" };
        res.status = httplib::StatusCode::InternalServerError_500;
        return;
    }

    const std::int64_t user_id{ su::string_to_int(req.get_file_value("user_id").content) };
    if (db.is_super_admin(user_id)) {
        logging::error{ "Trying to delete a super admin \"{}\"", user_id };
        res.status = httplib::StatusCode::Forbidden_403;
        return;
    }

    if (!db.delete_admin(user_id)) {
        logging::error{ "Fail to delete admin \"{}\"", user_id };
        return;
    }
}

inline void server::delete_user(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept
{
    if (!req.has_file("user_id")) {
        logging::error{ "Missing multipart form data" };
        res.status = httplib::StatusCode::InternalServerError_500;
        return;
    }

    const std::int64_t user_id{ su::string_to_int(req.get_file_value("user_id").content) };
    if (!db.delete_user(user_id)) {
        logging::error{ "Fail to delete user \"{}\"", user_id };
        return;
    }
}

inline void server::is_valid_user(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept
{
    if (!req.has_file("password") || !req.has_file("user_id")) {
        logging::error{ "Missing multipart form data" };
        res.status = httplib::StatusCode::InternalServerError_500;
        return;
    }

    const std::int64_t user_id{ su::string_to_int(req.get_file_value("user_id").content) };
    const std::string salt{ db.user_salt(user_id) };
    const std::string password{ crypto::password(req.get_file_value("password").content, salt) };

    const std::string database_password{ db.user_password(user_id) };
    res.set_content(su::bool_to_string(password == database_password), "plain/text");
}

inline void server::user_list(const httplib::Request& /*req*/, httplib::Response& res, const Database& db) noexcept
{
    const std::vector user_ids{ transform(db.user_list()) };
    res.set_content(su::join(user_ids), "plain/text");
}

inline void server::admin_list(const httplib::Request& /*req*/, httplib::Response& res, const Database& db) noexcept
{
    const std::vector user_ids{ transform(db.admin_list()) };
    res.set_content(su::join(user_ids), "plain/text");
}

inline void server::add_video(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept
{
    if (!req.has_file("title") || !req.has_file("video") || !req.has_file("user_ids")) {
        logging::error{ "Missing multipart form data" };
        res.status = httplib::StatusCode::InternalServerError_500;
        return;
    }

    const std::string video_title{ req.get_file_value("title").content };
    const std::string video_content{ req.get_file_value("video").content };
    const std::vector allowed_user_ids{ su::split(req.get_file_value("user_ids").content) };

    const std::optional success{
        db.add_video(video_title, video_content)
            .transform([&](const std::int64_t& video_id) -> bool {
                return db.add_video_rights(video_id, transform(allowed_user_ids));
            })
    };

    if (!success.value_or(false)) {
        logging::error{ "Fail to add video \"{}\"", video_title };
        return;
    }
}

inline void server::update_video(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept
{
    if (!req.has_file("user_ids")) {
        logging::error{ "Missing multipart form data" };
        res.status = httplib::StatusCode::InternalServerError_500;
        return;
    }

    const std::int64_t video_id{ su::string_to_int(req.path_params.at("video_id")) };

    const std::vector allowed_user_ids{ su::split(req.get_file_value("user_ids").content) };
    if (!db.update_video_rights(video_id, transform(allowed_user_ids))) {
        logging::error{ "Fail to update video \"{}\"", video_id };
        return;
    }
}

inline void server::delete_video(const httplib::Request& req, httplib::Response& /*res*/, const Database& db) noexcept
{
    const std::int64_t video_id{ su::string_to_int(req.path_params.at("video_id")) };
    if (!db.delete_video(video_id)) {
        logging::error{ "Fail to delete video \"{}\"", video_id };
        return;
    }
}

inline void server::increment_video_views(const httplib::Request& req, httplib::Response& /*res*/, const Database& db) noexcept
{
    const std::int64_t video_id{ su::string_to_int(req.path_params.at("video_id")) };
    if (!db.increment_video_views(video_id)) {
        logging::error{ "Fail to increment video views \"{}\"", video_id };
        return;
    }
}

inline void server::video(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept
{
    const std::int64_t video_id{ su::string_to_int(req.path_params.at("video_id")) };
    const std::string video_content{ db.video(video_id) };

    static constexpr std::size_t DATA_CHUNK_SIZE{ 4 * 1024 };
    res.set_content_provider(
        video_content.size(), // Content length
        "video/mp4",          // Content type
        [video_content](std::size_t offset, std::size_t length, httplib::DataSink& sink) {
            sink.write(&video_content[offset], std::min(length, DATA_CHUNK_SIZE));
            return true; // return 'false' if you want to cancel the process.
        },
        [](bool /*success*/) { /*release*/ });
}

inline void server::has_video_right(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept
{
    const std::int64_t video_id{ su::string_to_int(req.path_params.at("video_id")) };

    bool has_video_right{ false };
    if (req.has_header("user_id")) {
        const std::int64_t user_id{ su::string_to_int(req.get_header_value("user_id")) };
        has_video_right = db.has_video_right(video_id, user_id);
    } else {
        has_video_right = db.has_video_right(video_id);
    }

    res.set_content(su::bool_to_string(has_video_right), "plain/text");
}

inline void server::video_right_list(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept
{
    const std::int64_t video_id{ su::string_to_int(req.path_params.at("video_id")) };

    const std::vector rights{ transform(db.video_right_list(video_id)) };
    res.set_content(su::join(rights), "plain/text");
}
