#include "server.h"

#include "chunkworker.h"
#include "crypto.h"
#include "database.h"
#include "filesystem.h"
#include "logging.h"
#include "search.h"
#include "servercommon.h"
#include "stringutils.h"
#include "video.h"

#include <httplib.h>

namespace server
{
    bool create_super_admin(const Database& db) noexcept;

    void template_page(const httplib::Request& req, httplib::Response& res) noexcept;
    void static_file(const httplib::Request& req, httplib::Response& res) noexcept;

    void admin_video_list(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept;
    void user_video_list(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept;

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
    void update_user(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept;
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
    void video_size(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept;
    void thumbnail(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept;
    void has_video_right(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept;

    void video_right_list(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept;
}

int server::start() noexcept
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
    server.set_logger([](const httplib::Request& req, const httplib::Response& res) noexcept {
        logging::raw_log(sc::log(req, res));
    });

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

inline bool server::create_super_admin(const Database& db) noexcept
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
    std::vector<std::string> transform(const std::vector<int>& list) noexcept;
    std::vector<int> transform(const std::vector<std::string>& list) noexcept;
    std::vector<int> extract(const Database& db, const std::string& search, const std::vector<int>& ids) noexcept;
}

inline std::vector<std::string> server::transform(const std::vector<int>& list) noexcept
{
    std::vector<std::string> str_list(list.size());
    std::ranges::transform(list, str_list.begin(), su::int_to_string);
    return str_list;
}

inline std::vector<int> server::transform(const std::vector<std::string>& list) noexcept
{
    std::vector<int> int_list(list.size());
    std::ranges::transform(list, int_list.begin(), su::string_to_int);
    return int_list;
}

inline std::vector<int> server::extract(const Database& db, const std::string& search, const std::vector<int>& ids) noexcept
{
    std::unordered_map<std::string, int> title_ids;
    title_ids.reserve(ids.size());
    std::ranges::transform(ids, std::inserter(title_ids, title_ids.end()),
                           [&db](int id) noexcept -> decltype(title_ids)::value_type {
                               return std::make_pair(db.video_title(id), id);
                           });
    return search::extract(search, title_ids);
}

inline void server::admin_video_list(const httplib::Request& /*req*/, httplib::Response& res, const Database& db) noexcept
{
    const std::vector ids{ db.admin_video_list() };
    const std::vector str_ids{ transform(ids) };
    res.set_content(su::join(str_ids), "plain/text");
}

inline void server::user_video_list(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept
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

inline void server::video_title(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept
{
    const int video_id{ su::string_to_int(req.path_params.at("video_id")) };
    const std::string video_title{ db.video_title(video_id) };
    res.set_content(video_title, "plain/text");
}

inline void server::video_views(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept
{
    const int video_id{ su::string_to_int(req.path_params.at("video_id")) };
    const int video_views{ db.video_views(video_id) };
    res.set_content(su::int_to_string(video_views), "plain/text");
}

inline void server::user_count(const httplib::Request& /*req*/, httplib::Response& res, const Database& db) noexcept
{
    const int count{ db.user_count() };
    res.set_content(su::int_to_string(count), "plain/text");
}

inline void server::video_count(const httplib::Request& /*req*/, httplib::Response& res, const Database& db) noexcept
{
    const int count{ db.video_count() };
    res.set_content(su::int_to_string(count), "plain/text");
}

inline void server::view_count(const httplib::Request& /*req*/, httplib::Response& res, const Database& db) noexcept
{
    const int count{ db.view_count() };
    res.set_content(su::int_to_string(count), "plain/text");
}

inline void server::is_admin(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept
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

inline void server::is_super_admin(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept
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

inline void server::is_user(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept
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

inline void server::user_name(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept
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

inline void server::user_id(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept
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
    const std::optional admin_id{ db.add_admin(username, password, salt) };
    if (!admin_id.has_value()) {
        logging::error{ R"(Fail to add admin "{}")", username };
        res.status = httplib::StatusCode::InternalServerError_500;
        return;
    }

    res.set_content(su::int_to_string(admin_id.value()), "plan/text");
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
    const std::optional user_id{ db.add_user(username, password, salt) };
    if (!user_id.has_value()) {
        logging::error{ R"(Fail to add user "{}")", username };
        res.status = httplib::StatusCode::InternalServerError_500;
        return;
    }

    res.set_content(su::int_to_string(user_id.value()), "plan/text");
}

inline void server::update_user(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept
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
            .and_then([&](int user_id) noexcept -> std::optional<int> {
                return db.update_user_password(user_id, password);
            })
    };

    if (!success_user_id.has_value()) {
        logging::error{ R"(Fail to update user "{}")", user_id };
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

    const int user_id{ su::string_to_int(req.get_file_value("user_id").content) };
    if (!db.delete_user(user_id)) {
        logging::error{ R"(Fail to delete user "{}")", user_id };
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

    const int user_id{ su::string_to_int(req.get_file_value("user_id").content) };
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
    const std::string thumbnail_content{ video::extract_first_frame(video_content) };

    const std::optional video_id{
        db.add_video(video_title, video_content)
            .and_then([&](int video_id) noexcept -> std::optional<int> {
                return db.add_video_thumbnail(video_id, thumbnail_content);
            })
            .and_then([&](int video_id) noexcept -> std::optional<int> {
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

inline void server::update_video(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept
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
            .transform([&](int video_id) noexcept -> bool {
                return db.update_video_rights(video_id, transform(allowed_user_ids));
            })
    };

    if (!success.value_or(false)) {
        logging::error{ R"(Fail to update video "{}")", video_id };
        return;
    }
}

inline void server::delete_video(const httplib::Request& req, httplib::Response& /*res*/, const Database& db) noexcept
{
    const int video_id{ su::string_to_int(req.path_params.at("video_id")) };
    if (!db.delete_video(video_id)) {
        logging::error{ R"(Fail to delete video "{}")", video_id };
        return;
    }
}

inline void server::increment_video_views(const httplib::Request& req, httplib::Response& /*res*/, const Database& db) noexcept
{
    const int video_id{ su::string_to_int(req.path_params.at("video_id")) };
    if (!db.increment_video_views(video_id)) {
        logging::error{ R"(Fail to increment video views "{}")", video_id };
        return;
    }
}

inline void server::video(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept
{
    const int video_id{ su::string_to_int(req.path_params.at("video_id")) };
    const int video_size{ db.video_size(video_id) };

    std::unique_ptr chunk_worker{ std::make_unique<ChunkWorker>() };
    chunk_worker->set_buffer_size(video_size);
    chunk_worker->start_chunk_at(req.get_header_value("Range"));

    ChunkWorker* const raw_chunk_worker{ chunk_worker.get() };

    chunk_worker->set_fetch_async_callback([raw_chunk_worker, video_id, &db]() noexcept -> bool {
        return db.video(video_id, raw_chunk_worker->chunk_offset(), raw_chunk_worker->append_chunk_callback());
    });

    chunk_worker->start_fetch_async();

    chunk_worker->wait_for_chunk();

    res.set_content_provider(
        video_size,  // Content length
        "video/mp4", // Content type
        [raw_chunk_worker](std::size_t /*offset*/, std::size_t /*length*/, httplib::DataSink& sink) noexcept -> bool {
            const std::string chunk{ raw_chunk_worker->chunk() };
            sink.write(chunk.data(), chunk.size());
            return raw_chunk_worker->fetch_result(); // return 'false' will cancel the process.
        },
        sc::ContentProviderReleaser{ std::move(chunk_worker) });
}

inline void server::video_size(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept
{
    const int video_id{ su::string_to_int(req.path_params.at("video_id")) };
    const int video_size{ db.video_size(video_id) };
    res.set_content(su::int_to_string(video_size), "plain/text");
}

inline void server::thumbnail(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept
{
    const int video_id{ su::string_to_int(req.path_params.at("video_id")) };
    std::unique_ptr thumbnail_content{ std::make_unique<std::string>(db.thumbnail(video_id)) };
    const std::string* const raw_thumbnail_content{ thumbnail_content.get() };

    static constexpr std::size_t data_chunk_size{ static_cast<std::size_t>(4LL * 1024LL) };
    res.set_content_provider(
        raw_thumbnail_content->size(), // Content length
        "image/png",                   // Content type
        [raw_thumbnail_content](std::size_t offset, std::size_t length, httplib::DataSink& sink) noexcept -> bool {
            sink.write(&(*raw_thumbnail_content)[offset], std::min(length, data_chunk_size));
            return true; // return 'false' if you want to cancel the process.
        },
        sc::ContentProviderReleaser{ std::move(thumbnail_content) });
}

inline void server::has_video_right(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept
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

inline void server::video_right_list(const httplib::Request& req, httplib::Response& res, const Database& db) noexcept
{
    const int video_id{ su::string_to_int(req.path_params.at("video_id")) };

    const std::vector rights{ transform(db.video_right_list(video_id)) };
    res.set_content(su::join(rights), "plain/text");
}
