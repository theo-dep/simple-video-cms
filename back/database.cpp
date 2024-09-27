#include "database.h"

#include "databaseschema.h"
#include "servercommon.h"
#include "stringutils.h"

#include <ormpp.hpp>

#include <memory>
#include <ranges>

namespace database
{
    using dbng = ormpp::dbng<ormpp::mysql>;
    std::unique_ptr<dbng> connection() noexcept;

    template <typename T>
    std::vector<T> transform(const std::vector<std::tuple<T>>& query_out) noexcept;

    std::optional<Admin> admin(const std::unique_ptr<dbng>& conn, const std::string& username) noexcept;
    std::optional<User> user(const std::unique_ptr<dbng>& conn, const std::string& username) noexcept;
    std::optional<Video> video(const std::unique_ptr<dbng>& conn, const std::string& id) noexcept;
}

inline std::unique_ptr<database::dbng> database::connection() noexcept
{
    std::unique_ptr conn{ std::make_unique<dbng>() };
    return (conn->connect(sc::get_env("MYSQL_DB_URL", "localhost").c_str(),
                          sc::get_env("MYSQL_ROOT_USER", "root").c_str(),
                          sc::get_env("MYSQL_ROOT_PASSWORD", "1234").c_str(),
                          sc::get_env("MYSQL_DB_NAME", "video").c_str(),
                          /*timeout*/ 0,
                          su::string_to_int(sc::get_env("MYSQL_DB_PORT", "3306")))
                ? std::move(conn)
                : nullptr);
}

bool database::create_tables() noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        ERR("Fail to open database connection");
        return false;
    }

    {
        ormpp_auto_key key{ "id" };
        ormpp_not_null not_null{ { "username", "password" } };
        if (!conn->create_datatable<Admin>(key, not_null)) {
            ERR("Fail to create Admin table");
            return false;
        }
        if (!conn->create_datatable<User>(key, not_null)) {
            ERR("Fail to create User table");
            return false;
        }
    }
    {
        ormpp_key key{ "id" };
        ormpp_not_null not_null{ { "id" } };
        if (!conn->create_datatable<Video>(key, not_null)) {
            ERR("Fail to create Video table");
            return false;
        }
    }

    return true;
}

bool database::is_open() noexcept
{
    const std::unique_ptr conn{ connection() };
    return (conn != nullptr);
}

template <typename T>
std::vector<T> database::transform(const std::vector<std::tuple<T>>& query_out) noexcept
{
    constexpr auto transform_func{
        []<typename U>(const std::tuple<U>& v) constexpr -> U { return std::get<0>(v); }
    };

    std::vector<T> out(query_out.size());
    std::ranges::copy(std::views::transform(query_out, transform_func), out.begin());
    return out;
}

std::vector<std::string> database::most_viewed() noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        ERR("Fail to open database connection");
        return {};
    }

    const std::vector tuple_ids{ conn->query_s<std::tuple<std::string>>("SELECT id FROM Video ORDER BY view_count DESC LIMIT 10") };
    return transform(tuple_ids);
}

inline std::optional<Video> database::video(const std::unique_ptr<dbng>& conn, const std::string& id) noexcept
{
    const std::vector videos{ conn->query_s<Video>("id=?", id) };
    return (videos.empty() ? std::nullopt : std::optional{ videos[0] });
}

std::string database::video_title(const std::string& video_id) noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        ERR("Fail to open database connection");
        return {};
    }

    return video(conn, video_id).value_or(Video{}).title;
}

int database::video_views(const std::string& video_id) noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        ERR("Fail to open database connection");
        return -1;
    }

    return video(conn, video_id).value_or(Video{}).view_count;
}

std::string database::video_uploader(const std::string& video_id) noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        ERR("Fail to open database connection");
        return {};
    }

    return video(conn, video_id).value_or(Video{}).uploader;
}

std::optional<Admin> database::admin(const std::unique_ptr<dbng>& conn, const std::string& username) noexcept
{
    const std::vector admins{ conn->query_s<Admin>("username=?", username) };
    return (admins.empty() ? std::nullopt : std::optional{ admins[0] });
}

std::optional<User> database::user(const std::unique_ptr<dbng>& conn, const std::string& username) noexcept
{
    const std::vector users{ conn->query_s<User>("username=?", username) };
    return (users.empty() ? std::nullopt : std::optional{ users[0] });
}

bool database::is_admin(const std::string& username) noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        ERR("Fail to open database connection");
        return false;
    }

    return admin(conn, username).has_value();
}

bool database::is_user(const std::string& username) noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        ERR("Fail to open database connection");
        return false;
    }

    return user(conn, username).has_value();
}

void database::add_user(const std::string& username, const std::string& password) noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        ERR("Fail to open database connection");
        return;
    }

    try {
        User user;
        user.username = username;
        user.password = password;
        conn->insert(user);
    } catch (const std::exception& e) {
        ERR("Fail to insert user \"{}\" with error: {}", username, e.what());
    }
}

void database::add_admin(const std::string& username, const std::string& password) noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        ERR("Fail to open database connection");
        return;
    }

    try {
        Admin admin;
        admin.username = username;
        admin.password = password;
        conn->insert(admin);
    } catch (const std::exception& e) {
        ERR("Fail to insert admin \"{}\" with error: {}", username, e.what());
    }
}

std::string database::get_password(const std::string& username) noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        ERR("Fail to open database connection");
        return {};
    }

    if (const std::optional u{ user(conn, username) }; u.has_value()) {
        return u->password;
    }

    if (const std::optional user{ admin(conn, username) }; user.has_value()) {
        return user->password;
    }

    return {};
}

int database::user_count() noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        ERR("Fail to open database connection");
        return -1;
    }

    return static_cast<int>(conn->query_s<User>().size());
}

int database::video_count() noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        ERR("Fail to open database connection");
        return -1;
    }

    return static_cast<int>(conn->query_s<Video>().size());
}

int database::view_count() noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        ERR("Fail to open database connection");
        return -1;
    }

    const std::vector view_counts{ conn->query_s<std::tuple<int>>("SELECT SUM(view_count) FROM Video") };
    return (view_counts.empty() ? -1 : std::get<0>(view_counts[0]));
}

std::vector<std::string> database::user_list() noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        ERR("Fail to open database connection");
        return {};
    }

    const std::vector tuple_usernames{ conn->query_s<std::tuple<std::string>>("SELECT username FROM User") };
    return transform(tuple_usernames);
}
