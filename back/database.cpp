#include "database.h"

#include "databaseschema.h"
#include "logging.h"
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
    std::optional<Video> video(const std::unique_ptr<dbng>& conn, const types::md5_varchar& id) noexcept;
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
        logging::error{ "Fail to open database connection" };
        return false;
    }

    {
        const ormpp_auto_key key{ "id" };
        const ormpp_not_null not_null{ { "username", "password", "salt" } };
        // const ormpp_unique unique{ { "username" } }; // need varchar
        if (!conn->create_datatable<Admin>(key, not_null /*, unique*/)) {
            logging::error{ "Fail to create Admin table" };
            return false;
        }
        if (!conn->create_datatable<User>(key, not_null /*, unique*/)) {
            logging::error{ "Fail to create User table" };
            return false;
        }
    }
    {
        const ormpp_key key{ "id" };
        const ormpp_not_null not_null{ { "id", "title", "file_path" } };
        if (!conn->create_datatable<Video>(key, not_null)) {
            logging::error{ "Fail to create Video table" };
            return false;
        }
    }
    {
        const ormpp_not_null not_null{ { "video_id", "user_id" } };
        if (!conn->create_datatable<VideoRight>(not_null)) {
            logging::error{ "Fail to create VideoRight table" };
            return false;
        }
        if (!conn->execute("ALTER TABLE `VideoRight` ADD UNIQUE KEY (video_id, user_id)")) {
            logging::error{ "Fail to create VideoRight unique key" };
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

std::vector<types::md5_varchar> database::video_list() noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        logging::error{ "Fail to open database connection" };
        return {};
    }

    const std::vector tuple_videos{ conn->query_s<std::tuple<types::md5_varchar>>("SELECT id FROM Video") };
    return transform(tuple_videos);
}

std::vector<types::md5_varchar> database::video_list(const std::string& username) noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        logging::error{ "Fail to open database connection" };
        return {};
    }

    const int user_id{ user(conn, username).value_or(User{}).id };
    const std::vector tuple_videos{
        conn->query_s<std::tuple<types::md5_varchar>>("SELECT id FROM Video "
                                                      "WHERE id IN ("
                                                      "SELECT video_id FROM VideoRight "
                                                      "WHERE user_id=?"
                                                      ")",
                                                      user_id)
    };
    return transform(tuple_videos);
}

std::vector<types::md5_varchar> database::no_right_video_list() noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        logging::error{ "Fail to open database connection" };
        return {};
    }

    const std::vector tuple_videos{
        conn->query_s<std::tuple<types::md5_varchar>>("SELECT id FROM Video "
                                                      "WHERE id NOT IN ("
                                                      "SELECT video_id FROM VideoRight"
                                                      ")")
    };
    return transform(tuple_videos);
}

inline std::optional<Video> database::video(const std::unique_ptr<dbng>& conn, const types::md5_varchar& id) noexcept
{
    const std::vector videos{ conn->query_s<Video>("id=?", id) };
    return (videos.empty() ? std::nullopt : std::optional{ videos[0] });
}

std::string database::video_title(const types::md5_varchar& id) noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        logging::error{ "Fail to open database connection" };
        return {};
    }

    return video(conn, id).value_or(Video{}).title;
}

int database::video_views(const types::md5_varchar& id) noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        logging::error{ "Fail to open database connection" };
        return -1;
    }

    return video(conn, id).value_or(Video{}).view_count;
}

std::string database::video_file_path(const types::md5_varchar& id) noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        logging::error{ "Fail to open database connection" };
        return {};
    }

    return video(conn, id).value_or(Video{}).file_path;
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

void database::add_super_admin(const std::string& username, const std::string& password, const std::string& salt) noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        logging::error{ "Fail to open database connection" };
        return;
    }

    Admin admin;
    admin.username = username;
    admin.password = password;
    admin.salt = salt;
    admin.super = true;

    try {
        conn->insert(admin);
    } catch (const std::exception& e) {
        logging::error{ "Fail to insert super admin \"{}\" with error: {}", username, e.what() };
    }
}

bool database::is_super_admin(const std::string& username) noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        logging::error{ "Fail to open database connection" };
        return false;
    }

    return admin(conn, username).value_or(Admin{}).super;
}

bool database::is_admin(const std::string& username) noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        logging::error{ "Fail to open database connection" };
        return false;
    }

    return admin(conn, username).has_value();
}

bool database::is_user(const std::string& username) noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        logging::error{ "Fail to open database connection" };
        return false;
    }

    return user(conn, username).has_value();
}

void database::add_admin(const std::string& username, const std::string& password, const std::string& salt) noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        logging::error{ "Fail to open database connection" };
        return;
    }

    Admin admin;
    admin.username = username;
    admin.password = password;
    admin.salt = salt;

    try {
        conn->insert(admin);
    } catch (const std::exception& e) {
        logging::error{ "Fail to insert admin \"{}\" with error: {}", username, e.what() };
    }
}

void database::add_user(const std::string& username, const std::string& password, const std::string& salt) noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        logging::error{ "Fail to open database connection" };
        return;
    }

    User user;
    user.username = username;
    user.password = password;
    user.salt = salt;

    try {
        conn->insert(user);
    } catch (const std::exception& e) {
        logging::error{ "Fail to insert user \"{}\" with error: {}", username, e.what() };
    }
}

void database::update_admin(const std::string& username, const std::string& password) noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        logging::error{ "Fail to open database connection" };
        return;
    }

    // get previous admin (update with where clause not available)
    std::optional admin_to_update{ admin(conn, username) };
    if (!admin_to_update.has_value()) {
        logging::error{ "Unknown admin: {}", username };
        return;
    }

    admin_to_update->password = password;

    try {
        conn->update_some<&Admin::password>(admin_to_update.value());
    } catch (const std::exception& e) {
        logging::error{ "Fail to update admin \"{}\" with error: {}", username, e.what() };
    }
}

void database::update_user(const std::string& username, const std::string& password) noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        logging::error{ "Fail to open database connection" };
        return;
    }

    // get previous user (update with where clause not available)
    std::optional user_to_update{ user(conn, username) };
    if (!user_to_update.has_value()) {
        logging::error{ "Unknown user: {}", username };
        return;
    }

    user_to_update->password = password;

    try {
        conn->update_some<&User::password>(user_to_update.value());
    } catch (const std::exception& e) {
        logging::error{ "Fail to update user \"{}\" with error: {}", username, e.what() };
    }
}

void database::delete_admin(const std::string& username) noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        logging::error{ "Fail to open database connection" };
        return;
    }

    conn->delete_records_s<Admin>("username=?", username);
}

void database::delete_user(const std::string& username) noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        logging::error{ "Fail to open database connection" };
        return;
    }

    conn->delete_records_s<User>("username=?", username);
}

std::string database::user_password(const std::string& username) noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        logging::error{ "Fail to open database connection" };
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

std::string database::user_salt(const std::string& username) noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        logging::error{ "Fail to open database connection" };
        return {};
    }

    if (const std::optional u{ user(conn, username) }; u.has_value()) {
        return u->salt;
    }

    if (const std::optional user{ admin(conn, username) }; user.has_value()) {
        return user->salt;
    }

    return {};
}

int database::user_count() noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        logging::error{ "Fail to open database connection" };
        return -1;
    }

    return static_cast<int>(conn->query_s<User>().size());
}

int database::video_count() noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        logging::error{ "Fail to open database connection" };
        return -1;
    }

    return static_cast<int>(conn->query_s<Video>().size());
}

int database::view_count() noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        logging::error{ "Fail to open database connection" };
        return -1;
    }

    const std::vector view_counts{ conn->query_s<std::tuple<int>>("SELECT SUM(view_count) FROM Video") };
    return (view_counts.empty() ? -1 : std::get<0>(view_counts[0]));
}

std::vector<std::string> database::user_list() noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        logging::error{ "Fail to open database connection" };
        return {};
    }

    const std::vector tuple_usernames{ conn->query_s<std::tuple<std::string>>("SELECT username FROM User") };
    return transform(tuple_usernames);
}

std::vector<std::string> database::admin_list() noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        logging::error{ "Fail to open database connection" };
        return {};
    }

    const std::vector tuple_usernames{ conn->query_s<std::tuple<std::string>>("SELECT username FROM Admin") };
    return transform(tuple_usernames);
}

void database::add_video(const types::md5_varchar& id, const std::string& title, const std::string& file_path) noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        logging::error{ "Fail to open database connection" };
        return;
    }

    const Video video{
        .id = id,
        .title = title,
        .file_path = file_path
    };

    try {
        conn->insert(video);
    } catch (const std::exception& e) {
        logging::error{ "Fail to insert video \"{}\", \"{}\" with error: {}", id, title, e.what() };
    }
}

void database::add_video_rights(const types::md5_varchar& id, const std::vector<std::string>& usernames) noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        logging::error{ "Fail to open database connection" };
        return;
    }

    std::vector<VideoRight> video_usernames(usernames.size());
    std::ranges::transform(usernames, video_usernames.begin(),
                           [&id, &conn](const std::string& username) -> VideoRight {
                               return VideoRight{
                                   .video_id = id,
                                   .user_id = user(conn, username).value_or(User{}).id
                               };
                           });

    try {
        conn->insert(video_usernames);
    } catch (const std::exception& e) {
        logging::error{ "Fail to insert video rights \"{}\" with error: {}", id, e.what() };
    }
}

void database::update_video_rights(const types::md5_varchar& id, const std::vector<std::string>& usernames) noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        logging::error{ "Fail to open database connection" };
        return;
    }

    conn->delete_records_s<VideoRight>("video_id=?", id);

    add_video_rights(id, usernames);
}

void database::delete_video(const types::md5_varchar& id) noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        logging::error{ "Fail to open database connection" };
        return;
    }

    conn->delete_records_s<Video>("id=?", id);
}

void database::increment_video_views(const types::md5_varchar& id) noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        logging::error{ "Fail to open database connection" };
        return;
    }

    // get previous user (update with where clause not available)
    std::optional video_to_update{ video(conn, id) };
    if (!video_to_update.has_value()) {
        logging::error{ "Unknown video: {}", id };
        return;
    }

    video_to_update->view_count += 1;

    try {
        conn->update_some<&Video::view_count>(video_to_update.value());
    } catch (const std::exception& e) {
        logging::error{ "Fail to update video \"{}\" with error: {}", id, e.what() };
    }
}

bool database::has_video_right(const types::md5_varchar& id) noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        logging::error{ "Fail to open database connection" };
        return false;
    }

    const std::vector videos_rights{ conn->query_s<VideoRight>("video_id=?", id) };
    return videos_rights.empty();
}

bool database::has_video_right(const types::md5_varchar& id, const std::string& username) noexcept
{
    if (has_video_right(id))
        return true;

    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        logging::error{ "Fail to open database connection" };
        return false;
    }

    // check if user is admin
    if (admin(conn, username).has_value())
        return true;

    const int user_id{ user(conn, username).value_or(User{}).id };
    const std::vector video_usernames{ conn->query_s<VideoRight>("video_id=? AND user_id=?", id, user_id) };
    return !video_usernames.empty();
}

std::vector<std::string> database::video_right_list(const types::md5_varchar& id) noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        logging::error{ "Fail to open database connection" };
        return {};
    }

    const std::vector tuple_rights{
        conn->query_s<std::tuple<std::string>>(
            "SELECT username FROM User "
            "WHERE id IN (SELECT user_id FROM VideoRight WHERE video_id=?)",
            id)
    };
    return transform(tuple_rights);
}
