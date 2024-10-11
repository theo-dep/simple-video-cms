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
    void create_admin() noexcept;

    void template_page(const httplib::Request& req, httplib::Response& res) noexcept;

    void video_list(const httplib::Request& req, httplib::Response& res) noexcept;
    void most_viewed(const httplib::Request& req, httplib::Response& res) noexcept;

    void video_title(const httplib::Request& req, httplib::Response& res) noexcept;
    void video_views(const httplib::Request& req, httplib::Response& res) noexcept;

    void user_count(const httplib::Request& req, httplib::Response& res) noexcept;
    void video_count(const httplib::Request& req, httplib::Response& res) noexcept;
    void view_count(const httplib::Request& req, httplib::Response& res) noexcept;

    void is_admin(const httplib::Request& req, httplib::Response& res) noexcept;
    void is_valid_username(const httplib::Request& req, httplib::Response& res) noexcept;

    void add_user(const httplib::Request& req, httplib::Response& res) noexcept;
    void update_user(const httplib::Request& req, httplib::Response& res) noexcept;
    void delete_user(const httplib::Request& req, httplib::Response& res) noexcept;
    void is_valid_user(const httplib::Request& req, httplib::Response& res) noexcept;

    void user_list(const httplib::Request& req, httplib::Response& res) noexcept;

    void add_video(const httplib::Request& req, httplib::Response& res) noexcept;
    void delete_video(const httplib::Request& req, httplib::Response& res) noexcept;
    void increment_video_views(const httplib::Request& req, httplib::Response& res) noexcept;
    void video(const httplib::Request& req, httplib::Response& res) noexcept;
}

int server::start() noexcept
{
    if (!database::create_tables()) {
        logging::error{ "Fail to create database tables" };
        return EXIT_FAILURE;
    }

    create_admin();

    httplib::Server server;
    server.set_logger([](const httplib::Request& req, const httplib::Response& res) {
        logging::raw_log(sc::log(req, res));
    });

    server
        .Get("/html/:html", sc::serve(template_page))

        .Get("/video-list", sc::serve(video_list))
        .Get("/most-viewed", sc::serve(most_viewed))

        .Get("/title/:video_id", sc::serve(video_title))
        .Get("/views/:video_id", sc::serve(video_views))

        .Get("/user-count", sc::serve(user_count))
        .Get("/video-count", sc::serve(video_count))
        .Get("/view-count", sc::serve(view_count))

        .Get("/is-admin", sc::serve(is_admin))

        .Get("/is-valid-username", sc::serve(is_valid_username))

        .Post("/add-user", sc::serve(add_user))
        .Post("/update-user", sc::serve(update_user))
        .Post("/delete-user", sc::serve(delete_user))
        .Post("/is-valid-user", sc::serve(is_valid_user))

        .Get("/user-list", sc::serve(user_list))

        .Post("/add-video", sc::serve(add_video))
        .Post("/delete-video/:video_id", sc::serve(delete_video))
        .Post("/increment-video-views/:video_id", sc::serve(increment_video_views))
        .Get("/video/:video_id", sc::serve(video));

    constexpr const char* host{ "0.0.0.0" };
    constexpr int port{ 5000 };
    logging::info{ "Serving HTTP on {0} port {1} ...", host, port };
    return (server.listen(host, port) ? EXIT_SUCCESS : EXIT_FAILURE);
}

inline void server::create_admin() noexcept
{
    const std::string username{ sc::get_env("MYSQL_ADMIN_USERNAME", "admin") };
    const std::string password{ crypto::sha512(sc::get_env("MYSQL_ADMIN_PASSWORD", "admin")) };
    database::add_admin(username, password);
}

inline void server::template_page(const httplib::Request& req, httplib::Response& res) noexcept
{
    const std::string html{ req.path_params.at("html") };
    const std::filesystem::path html_path{ std::filesystem::current_path() / "templates" / html };
    res.set_file_content(html_path.string(), "text/html");
}

namespace server
{
    std::vector<std::string> transform(const std::vector<types::md5_varchar>& list) noexcept;
}

inline std::vector<std::string> server::transform(const std::vector<types::md5_varchar>& list) noexcept
{
    std::vector<std::string> str_list(list.size());
    std::ranges::transform(list, str_list.begin(), su::md5_varchar_to_string);
    return str_list;
}

inline void server::video_list(const httplib::Request& /*req*/, httplib::Response& res) noexcept
{
    const std::vector<std::string> ids{ transform(database::video_list()) };
    res.set_content(su::join(ids), "plain/text");
}

inline void server::most_viewed(const httplib::Request& /*req*/, httplib::Response& res) noexcept
{
    const std::vector<std::string> ids{ transform(database::most_viewed()) };
    res.set_content(su::join(ids), "plain/text");
}

inline void server::video_title(const httplib::Request& req, httplib::Response& res) noexcept
{
    const types::md5_varchar video_id{ su::string_to_md5_varchar(req.path_params.at("video_id")) };
    const std::string video_title{ database::video_title(video_id) };
    res.set_content(video_title, "plain/text");
}

inline void server::video_views(const httplib::Request& req, httplib::Response& res) noexcept
{
    const types::md5_varchar video_id{ su::string_to_md5_varchar(req.path_params.at("video_id")) };
    const int video_views{ database::video_views(video_id) };
    res.set_content(std::to_string(video_views), "plain/text");
}

inline void server::user_count(const httplib::Request& /*req*/, httplib::Response& res) noexcept
{
    const int count{ database::user_count() };
    res.set_content(std::to_string(count), "plain/text");
}

inline void server::video_count(const httplib::Request& /*req*/, httplib::Response& res) noexcept
{
    const int count{ database::video_count() };
    res.set_content(std::to_string(count), "plain/text");
}

inline void server::view_count(const httplib::Request& /*req*/, httplib::Response& res) noexcept
{
    const int count{ database::view_count() };
    res.set_content(std::to_string(count), "plain/text");
}

inline void server::is_admin(const httplib::Request& req, httplib::Response& res) noexcept
{
    if (!req.has_header("username")) {
        logging::error{ "Missing header data" };
        res.status = httplib::StatusCode::InternalServerError_500;
        return;
    }

    const std::string username{ req.get_header_value("username") };
    const bool is_admin{ database::is_admin(username) };
    res.set_content(su::bool_to_string(is_admin), "plain/text");
}

inline void server::is_valid_username(const httplib::Request& req, httplib::Response& res) noexcept
{
    if (!req.has_header("username")) {
        logging::error{ "Missing header data" };
        res.status = httplib::StatusCode::InternalServerError_500;
        return;
    }

    const std::string username{ req.get_header_value("username") };
    const bool is_valid_username{ !database::is_user(username) };
    res.set_content(su::bool_to_string(is_valid_username), "plain/text");
}

inline void server::add_user(const httplib::Request& req, httplib::Response& res) noexcept
{
    if (!req.has_file("password") || !req.has_file("username")) {
        logging::error{ "Missing multipart form data" };
        res.status = httplib::StatusCode::InternalServerError_500;
        return;
    }

    const std::string username{ req.get_file_value("username").content };
    const std::string password{ req.get_file_value("password").content };
    database::add_user(username, password);
}

inline void server::update_user(const httplib::Request& req, httplib::Response& res) noexcept
{
    if (!req.has_file("password") || !req.has_file("username")) {
        logging::error{ "Missing multipart form data" };
        res.status = httplib::StatusCode::InternalServerError_500;
        return;
    }

    const std::string username{ req.get_file_value("username").content };
    const std::string password{ req.get_file_value("password").content };
    database::update_user(username, password);
}

inline void server::delete_user(const httplib::Request& req, httplib::Response& res) noexcept
{
    if (!req.has_file("username")) {
        logging::error{ "Missing multipart form data" };
        res.status = httplib::StatusCode::InternalServerError_500;
        return;
    }

    const std::string username{ req.get_file_value("username").content };
    database::delete_user(username);
}

inline void server::is_valid_user(const httplib::Request& req, httplib::Response& res) noexcept
{
    if (!req.has_file("password") || !req.has_file("username")) {
        logging::error{ "Missing multipart form data" };
        res.status = httplib::StatusCode::InternalServerError_500;
        return;
    }

    const std::string username{ req.get_file_value("username").content };
    const std::string password{ req.get_file_value("password").content };

    const std::string database_password{ database::get_password(username) };
    res.set_content(su::bool_to_string(password == database_password), "plain/text");
}

inline void server::user_list(const httplib::Request& /*req*/, httplib::Response& res) noexcept
{
    const std::vector<std::string> usernames{ database::user_list() };
    res.set_content(su::join(usernames), "plain/text");
}

namespace server
{
    bool create_directories(const std::filesystem::path& path) noexcept;
    bool remove(const std::filesystem::path& path) noexcept;
    std::filesystem::path video_path() noexcept;
}

inline bool server::create_directories(const std::filesystem::path& path) noexcept
{
    if (std::filesystem::exists(path))
        return true;

    std::error_code create_error;
    if (!std::filesystem::create_directories(path, create_error) || create_error) {
        logging::error{ "Fail to create \"{}\" with error {}: \"{}\"", path.string(), create_error.value(), create_error.message() };
        return false;
    }

    return true;
}

inline bool server::remove(const std::filesystem::path& path) noexcept
{
    std::error_code create_error;
    if (!std::filesystem::remove(path, create_error) || create_error) {
        logging::error{ "Fail to remove \"{}\" with error {}: \"{}\"", path.string(), create_error.value(), create_error.message() };
        return false;
    }

    return true;
}

inline std::filesystem::path server::video_path() noexcept
{
    return std::filesystem::current_path() / "data" / "videos";
}

inline void server::add_video(const httplib::Request& req, httplib::Response& res) noexcept
{
    if (!req.has_file("title") || !req.has_file("video")) {
        logging::error{ "Missing multipart form data" };
        res.status = httplib::StatusCode::InternalServerError_500;
        return;
    }

    const std::string video_title{ req.get_file_value("title").content };
    const std::string video_content{ req.get_file_value("video").content };
    const types::md5_varchar video_id{ crypto::sha1(video_title) };

    const std::filesystem::path video_path{ server::video_path() };
    if (!server::create_directories(video_path)) {
        return;
    }

    const std::filesystem::path video_file_path{ video_path / su::md5_varchar_to_string(video_id) };
    {
        std::ofstream video_stream(video_file_path, std::ios::out | std::ios::binary | std::ios::trunc);
        video_stream.write(video_content.data(), video_content.size());
    }

    database::add_video(video_id, video_title, video_file_path.string());
}

inline void server::delete_video(const httplib::Request& req, httplib::Response& res) noexcept
{
    const std::string video_id_str{ req.path_params.at("video_id") };
    const types::md5_varchar video_id{ su::string_to_md5_varchar(video_id_str) };
    database::delete_video(video_id);

    // clean local data
    const std::filesystem::path video_file_path{ video_path() / video_id_str };
    if (!server::remove(video_file_path)) {
        res.status = httplib::StatusCode::InternalServerError_500;
    }
}

inline void server::increment_video_views(const httplib::Request& req, httplib::Response& /*res*/) noexcept
{
    const std::string video_id_str{ req.path_params.at("video_id") };
    const types::md5_varchar video_id{ su::string_to_md5_varchar(video_id_str) };
    database::increment_video_views(video_id);
}

inline void server::video(const httplib::Request& req, httplib::Response& res) noexcept
{
    const std::string video_id_str{ req.path_params.at("video_id") };
    const types::md5_varchar video_id{ su::string_to_md5_varchar(video_id_str) };
    const std::filesystem::path video_file_path{ database::video_file_path(video_id) };

    std::size_t video_length{};
    std::shared_ptr<char[]> video_content;
    {
        // https://insanecoding.blogspot.com/2011/11/how-to-read-in-file-in-c.html
        std::ifstream video_stream(video_file_path, std::ios::in | std::ios::binary);
        video_stream.seekg(0, std::ios::end);
        video_length = video_stream.tellg();
        video_content = std::make_shared_for_overwrite<char[]>(video_length);
        video_stream.seekg(0, std::ios::beg);
        video_stream.read(&video_content[0], video_length);
    }
    logging::debug{ "Video length: {}", video_length };

    res.set_content_provider(
        video_length, // Content length
        "video/mp4",  // Content type
        [video_content](std::size_t offset, std::size_t length, httplib::DataSink& sink) {
            sink.write(&video_content[offset], length);
            return true; // return 'false' if you want to cancel the process.
        },
        [video_content](bool /*success*/) { /*release*/ });
}
