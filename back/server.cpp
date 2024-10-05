#include "server.h"

#include "crypto.h"
#include "database.h"
#include "logging.h"
#include "servercommon.h"
#include "stringutils.h"

#include <httplib.h>

#include <filesystem>

namespace server
{
    void create_admin() noexcept;

    void template_page(const httplib::Request& req, httplib::Response& res) noexcept;

    void most_viewed(const httplib::Request& req, httplib::Response& res) noexcept;

    enum class EVideoStat : std::uint8_t
    {
        TITLE,
        VIEWS,
        UPLOADER
    };
    template <EVideoStat Stat>
    void video_stats(const httplib::Request& req, httplib::Response& res) noexcept;

    enum class EAdminCount : std::uint8_t
    {
        USERS,
        VIDEOS,
        VIEWS
    };
    template <EAdminCount Count>
    void admin_stats(const httplib::Request& req, httplib::Response& res) noexcept;

    void is_admin(const httplib::Request& req, httplib::Response& res) noexcept;
    void is_valid_username(const httplib::Request& req, httplib::Response& res) noexcept;

    void add_user(const httplib::Request& req, httplib::Response& res) noexcept;
    void update_user(const httplib::Request& req, httplib::Response& res) noexcept;
    void delete_user(const httplib::Request& req, httplib::Response& res) noexcept;
    void is_valid_user(const httplib::Request& req, httplib::Response& res) noexcept;

    void user_list(const httplib::Request& req, httplib::Response& res) noexcept;
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
        std::cout << sc::log(req, res) << std::endl;
    });

    server
        .Get("/html/:html", sc::serve(template_page))

        .Get("/most-viewed", sc::serve(most_viewed))

        .Get("/title/:video_id", sc::serve(video_stats<EVideoStat::TITLE>))
        .Get("/views/:video_id", sc::serve(video_stats<EVideoStat::VIEWS>))
        .Get("/uploader/:video_id", sc::serve(video_stats<EVideoStat::UPLOADER>))

        .Get("/user-count", sc::serve(admin_stats<EAdminCount::USERS>))
        .Get("/video-count", sc::serve(admin_stats<EAdminCount::VIDEOS>))
        .Get("/view-count", sc::serve(admin_stats<EAdminCount::VIEWS>))

        .Get("/is-admin", sc::serve(is_admin))

        .Get("/is-valid-username", sc::serve(is_valid_username))

        .Post("/add-user", sc::serve(add_user))
        .Post("/update-user", sc::serve(update_user))
        .Post("/delete-user", sc::serve(delete_user))
        .Post("/is-valid-user", sc::serve(is_valid_user))

        .Get("/user-list", sc::serve(user_list));

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

inline void server::most_viewed(const httplib::Request& /*req*/, httplib::Response& res) noexcept
{
    const std::vector<std::string> ids{ database::most_viewed() };
    res.set_content(su::join(ids), "plain/text");
}

template <server::EVideoStat Stat>
inline void server::video_stats(const httplib::Request& req, httplib::Response& res) noexcept
{
    const std::string video_id{ req.path_params.at("video_id") };

    if constexpr (Stat == EVideoStat::TITLE) {
        const std::string video_title{ database::video_title(video_id) };
        res.set_content(video_title, "plain/text");
    } else if constexpr (Stat == EVideoStat::VIEWS) {
        const int video_views{ database::video_views(video_id) };
        res.set_content(std::to_string(video_views), "plain/text");
    } else if constexpr (Stat == EVideoStat::UPLOADER) {
        const std::string video_uploader{ database::video_uploader(video_id) };
        res.set_content(video_uploader, "plain/text");
    } else {
        static_assert(false, "Stat not defined");
    }
}

template <server::EAdminCount Count>
inline void server::admin_stats(const httplib::Request& /*req*/, httplib::Response& res) noexcept
{
    int count{};

    if constexpr (Count == EAdminCount::USERS) {
        count = database::user_count();
    } else if constexpr (Count == EAdminCount::VIDEOS) {
        count = database::video_count();
    } else if constexpr (Count == EAdminCount::VIEWS) {
        count = database::view_count();
    } else {
        static_assert(false, "Count not defined");
    }

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
