#include "database.h"
#include "databaseschema.h"

#include "filesystem.h"
#include "logging.h"
#include "stringutils.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#pragma GCC diagnostic ignored "-Wunused-but-set-parameter"
#include <sqlite_orm.h>
#pragma GCC diagnostic pop

#include <algorithm>
#include <fstream>

using namespace sqlite_orm;

namespace database
{
    auto storage(const std::filesystem::path& path);
}

inline auto database::storage(const std::filesystem::path& path)
{
    return make_storage(
        path,
        make_table("users",
                   make_column("id", &User::id, primary_key().autoincrement()),
                   make_column("name", &User::name, unique()),
                   make_column("password", &User::password),
                   make_column("salt", &User::salt),
                   unique(&User::id, &User::name)),
        make_table("super_admins",
                   make_column("id", &SuperAdmin::id, primary_key()),
                   foreign_key(&SuperAdmin::id).references(&User::id).on_delete.cascade()),
        make_table("admins",
                   make_column("id", &Admin::id, primary_key()),
                   foreign_key(&Admin::id).references(&User::id).on_delete.cascade()),
        make_table("videos",
                   make_column("id", &Video::id, primary_key().autoincrement()),
                   make_column("title", &Video::title),
                   make_column("views", &Video::views)),
        make_table("video_rights",
                   make_column("video_id", &VideoRight::video_id),
                   make_column("user_id", &VideoRight::user_id),
                   primary_key(&VideoRight::video_id, &VideoRight::user_id),
                   foreign_key(&VideoRight::video_id).references(&Video::id).on_delete.cascade(),
                   foreign_key(&VideoRight::user_id).references(&User::id).on_delete.cascade()));
    // make virtual table with FTS5 for search?
    // https://www.sqlite.org/fts5.html
}

namespace database
{
    using StorageType = decltype(database::storage({}));

    int file_size(const std::string& path);
    std::string read_file(const std::string& path, std::size_t offset, std::size_t length);
    std::string read_file(const std::string& path);
    void write_file(const std::string& path, const std::string& content);
}

Database::Database(std::filesystem::path path)
    : path_{ std::move(path) }
{
}

bool Database::create_tables() const
{
    database::StorageType storage{ database::storage(path_) };
    storage.sync_schema();
    return true;
}

std::vector<int> Database::admin_video_list() const
{
    database::StorageType storage{ database::storage(path_) };
    return storage.select(&Video::id);
}

std::vector<int> Database::user_video_list(int user_id) const
{
    database::StorageType storage{ database::storage(path_) };
    return storage.select(distinct(&VideoRight::video_id), where(c(&VideoRight::user_id) == user_id));
}

std::vector<int> Database::no_user_video_list() const
{
    database::StorageType storage{ database::storage(path_) };
    return storage.select(distinct(&Video::id), from<Video>(), where(not_in(&Video::id, select(&VideoRight::video_id))));
}

std::string Database::video_title(int id) const
{
    database::StorageType storage{ database::storage(path_) };
    const std::optional video_title{
        storage.get_optional<Video>(id)
            .transform([](const Video& video) -> std::string {
                return video.title;
            })
            .or_else([&] -> std::optional<std::string> {
                logging::error{ R"(Fail to fetch video title "{}")", id };
                return std::string{};
            })
    };
    return video_title.value_or(std::string{});
}

int Database::video_views(int id) const
{
    database::StorageType storage{ database::storage(path_) };
    const std::optional video_views{
        storage.get_optional<Video>(id)
            .transform([](const Video& video) -> int {
                return video.views;
            })
            .or_else([&] -> std::optional<int> {
                logging::error{ R"(Fail to fetch video views "{}")", id };
                return -1;
            })
    };
    return video_views.value_or(-1);
}

int Database::video_size(int id) const
{
    database::StorageType storage{ database::storage(path_) };
    const std::optional video_size{
        storage.get_optional<Video>(id)
            .transform([&](const Video& video) -> int {
                return database::file_size(video_path(video.id));
            })
            .or_else([&] -> std::optional<int> {
                logging::error{ R"(Fail to fetch video size "{}")", id };
                return -1;
            })
    };
    return video_size.value_or(-1);
}

// maybe https://www.sqlite.org/fasterthanfs.html
// https://github.com/fnc12/sqlite_orm/blob/v1.9/examples/blob_binding.cpp
// https://github.com/fnc12/sqlite_orm/blob/v1.9/examples/key_value.cpp

std::string Database::video(int id, std::size_t offset, std::size_t length) const
{
    database::StorageType storage{ database::storage(path_) };
    const std::optional video{
        storage.get_optional<Video>(id)
            .transform([&](const Video& video) -> std::string {
                return database::read_file(video_path(video.id), offset, length);
            })
            .or_else([&] -> std::optional<std::string> {
                logging::error{ R"(Fail to fetch video content "{}")", id };
                return std::string{};
            })
    };
    return video.value_or(std::string{});
}

std::string Database::thumbnail(int id) const
{
    database::StorageType storage{ database::storage(path_) };
    const std::optional thumbnail{
        storage.get_optional<Video>(id)
            .transform([&](const Video& video) -> std::string {
                return database::read_file(thumbnail_path(video.id));
            })
            .or_else([&] -> std::optional<std::string> {
                logging::error{ R"(Fail to fetch video thumbnail "{}")", id };
                return std::string{};
            })
    };
    return thumbnail.value_or(std::string{});
}

std::optional<int> Database::add_super_admin(const std::string& name, const std::string& password, const std::string& salt) const
{
    User user_super_admin{
        .name = name,
        .password = password,
        .salt = salt
    };

    database::StorageType storage{ database::storage(path_) };
    user_super_admin.id = storage.insert(user_super_admin);

    const SuperAdmin super_admin{
        .id = user_super_admin.id
    };

    storage.replace(super_admin);
    return super_admin.id;
}

bool Database::is_super_admin(int id) const
{
    database::StorageType storage{ database::storage(path_) };
    return storage.get_all<SuperAdmin>(where(c(&SuperAdmin::id) == id)).size() == 1;
}

bool Database::is_admin(int id) const
{
    database::StorageType storage{ database::storage(path_) };
    return storage.get_all<User>(
                      where(
                          c(&User::id) == id and
                          (in(id, select(&Admin::id)) or
                           in(id, select(&SuperAdmin::id)))))
               .size() == 1;
}

bool Database::is_user(int id) const
{
    database::StorageType storage{ database::storage(path_) };
    return storage.get_all<User>(
                      where(
                          c(&User::id) == id and
                          not_in(id, select(&Admin::id)) and
                          not_in(id, select(&SuperAdmin::id))))
               .size() == 1;
}

std::optional<int> Database::add_admin(const std::string& name, const std::string& password, const std::string& salt) const
{
    User user_admin{
        .name = name,
        .password = password,
        .salt = salt
    };

    database::StorageType storage{ database::storage(path_) };
    user_admin.id = storage.insert(user_admin);

    const Admin admin{
        .id = user_admin.id
    };

    storage.replace(admin);
    return admin.id;
}

std::optional<int> Database::add_user(const std::string& name, const std::string& password, const std::string& salt) const
{
    User user{
        .name = name,
        .password = password,
        .salt = salt
    };

    database::StorageType storage{ database::storage(path_) };
    user.id = storage.insert(user);
    return user.id;
}

std::optional<int> Database::update_user_name(int id, const std::string& name) const
{
    database::StorageType storage{ database::storage(path_) };
    const std::optional user_id{
        storage.get_optional<User>(id)
            .and_then([&](User user) -> std::optional<int> {
                user.name = name;
                storage.update(user);
                return id;
            })
            .or_else([&] -> std::optional<int> {
                logging::error{ R"(Fail to update user name "{}")", id };
                return std::nullopt;
            })
    };
    return user_id;
}

std::optional<int> Database::update_user_password(int id, const std::string& password) const
{
    database::StorageType storage{ database::storage(path_) };
    const std::optional user_id{
        storage.get_optional<User>(id)
            .and_then([&](User user) -> std::optional<int> {
                user.password = password;
                storage.update(user);
                return id;
            })
            .or_else([&] -> std::optional<int> {
                logging::error{ R"(Fail to update user password "{}")", id };
                return std::nullopt;
            })
    };
    return user_id;
}

bool Database::delete_user(int id) const
{
    database::StorageType storage{ database::storage(path_) };
    const std::optional success{
        storage.get_optional<User>(id)
            .transform([&](const User& user) -> bool {
                storage.remove<User>(user.id);
                return true;
            })
            .or_else([&] -> std::optional<bool> {
                logging::error{ R"(Fail to delete user "{}")", id };
                return false;
            })
    };
    return success.value_or(false);
}

int Database::user_id(const std::string& name) const
{
    database::StorageType storage{ database::storage(path_) };
    const std::vector users{ storage.select(&User::id, where(c(&User::name) == name)) };
    return users.empty() ? -1 : users[0];
}

std::string Database::user_name(int id) const
{
    database::StorageType storage{ database::storage(path_) };
    const std::optional user_name{
        storage.get_optional<User>(id)
            .transform([](const User& user) -> std::string {
                return user.name;
            })
            .or_else([&] -> std::optional<std::string> {
                logging::error{ R"(Fail to fetch user name "{}")", id };
                return std::string{};
            })
    };
    return user_name.value_or(std::string{});
}

std::string Database::user_password(int id) const
{
    database::StorageType storage{ database::storage(path_) };
    const std::optional user_password{
        storage.get_optional<User>(id)
            .transform([](const User& user) -> std::string {
                return user.password;
            })
            .or_else([&] -> std::optional<std::string> {
                logging::error{ R"(Fail to fetch user password "{}")", id };
                return std::string{};
            })
    };
    return user_password.value_or(std::string{});
}

std::string Database::user_salt(int id) const
{
    database::StorageType storage{ database::storage(path_) };
    const std::optional user_salt{
        storage.get_optional<User>(id)
            .transform([](const User& user) -> std::string {
                return user.salt;
            })
            .or_else([&] -> std::optional<std::string> {
                logging::error{ R"(Fail to fetch user salt "{}")", id };
                return std::string{};
            })
    };
    return user_salt.value_or(std::string{});
}

int Database::user_count() const
{
    database::StorageType storage{ database::storage(path_) };
    return storage.count<User>();
}

int Database::video_count() const
{
    database::StorageType storage{ database::storage(path_) };
    return storage.count<Video>();
}

int Database::view_count() const
{
    database::StorageType storage{ database::storage(path_) };
    const std::unique_ptr sum{ storage.sum(&Video::views) };
    return sum ? *sum : 0;
}

std::vector<int> Database::user_list() const
{
    database::StorageType storage{ database::storage(path_) };
    return storage.select(except(select(&User::id), select(&Admin::id), select(&SuperAdmin::id)));
}

std::vector<int> Database::admin_list() const
{
    database::StorageType storage{ database::storage(path_) };
    return storage.select(union_all(select(&SuperAdmin::id), select(&Admin::id)));
}

std::optional<int> Database::add_video(const std::string& title, const std::string& video_content) const
{
    if (!filesystem::create(video_path())) {
        return std::nullopt;
    }

    Video video{
        .title = title
    };

    database::StorageType storage{ database::storage(path_) };
    video.id = storage.insert(video);

    storage.update(video);

    database::write_file(video_path(video.id), video_content);

    return video.id;
}

std::optional<int> Database::add_video_thumbnail(int id, const std::string& thumbnail_content) const
{
    if (!filesystem::create(thumbnail_path())) {
        return std::nullopt;
    }

    database::StorageType storage{ database::storage(path_) };
    const std::optional video_id{
        storage.get_optional<Video>(id)
            .transform([&](const Video& video) -> int {
                database::write_file(thumbnail_path(video.id), thumbnail_content);
                return video.id;
            })
    };
    return video_id;
}

bool Database::add_video_rights(int id, const std::vector<int>& user_ids) const
{
    std::vector<VideoRight> video_rights(user_ids.size());
    std::ranges::transform(user_ids, video_rights.begin(), [&id](int user_id) -> VideoRight {
        return VideoRight{ .video_id = id, .user_id = user_id };
    });

    database::StorageType storage{ database::storage(path_) };
    storage.replace_range(video_rights.cbegin(), video_rights.cend());
    return true;
}

std::optional<int> Database::update_video_title(int id, const std::string& title) const
{
    database::StorageType storage{ database::storage(path_) };
    const std::optional video_id{
        storage.get_optional<Video>(id)
            .and_then([&](Video video) -> std::optional<int> {
                video.title = title;
                storage.update(video);
                return id;
            })
            .or_else([&] -> std::optional<int> {
                logging::error{ R"(Fail to update video title "{}")", id };
                return std::nullopt;
            })
    };
    return video_id;
}

bool Database::update_video_rights(int id, const std::vector<int>& user_ids) const
{
    database::StorageType storage{ database::storage(path_) };
    storage.remove_all<VideoRight>(where(c(&VideoRight::video_id) == id));
    return add_video_rights(id, user_ids);
}

bool Database::delete_video(int id) const
{
    database::StorageType storage{ database::storage(path_) };
    const std::optional success{
        storage.get_optional<Video>(id)
            .and_then([&](const Video& video) -> std::optional<Video> {
                // remove video file
                return filesystem::remove(video_path(video.id)) ? std::optional(video) : std::nullopt;
            })
            .and_then([&](const Video& video) -> std::optional<Video> {
                // remove thumbnail file
                return filesystem::remove(thumbnail_path(video.id)) ? std::optional(video) : std::nullopt;
            })
            .transform([&](const Video& video) -> bool {
                storage.remove<Video>(video.id);
                return true;
            })
            .or_else([&] -> std::optional<bool> {
                logging::error{ R"(Fail to delete video "{}")", id };
                return false;
            })
    };
    return success.value_or(false);
}

bool Database::increment_video_views(int id) const
{
    database::StorageType storage{ database::storage(path_) };
    const std::optional success{
        storage.get_optional<Video>(id)
            .transform([&](Video video) -> bool {
                video.views += 1;
                storage.update(video);
                return true;
            })
            .or_else([&] -> std::optional<bool> {
                logging::error{ R"(Fail to increment video views "{}")", id };
                return false;
            })
    };
    return success.value_or(false);
}

bool Database::has_video_right(int id) const
{
    database::StorageType storage{ database::storage(path_) };
    return storage.get_all<VideoRight>(where(c(&VideoRight::video_id) == id)).empty();
}

bool Database::has_video_right(int id, int user_id) const
{
    database::StorageType storage{ database::storage(path_) };
    return !storage.get_all<VideoRight>(where(c(&VideoRight::video_id) == id and c(&VideoRight::user_id) == user_id)).empty();
}

std::vector<int> Database::video_right_list(int id) const
{
    database::StorageType storage{ database::storage(path_) };
    return storage.select(distinct(&VideoRight::user_id), where(c(&VideoRight::video_id) == id));
}

std::filesystem::path Database::base_path() const
{
    return path_.parent_path();
}

std::filesystem::path Database::video_path() const
{
    return base_path() / "videos";
}

std::filesystem::path Database::video_path(int id) const
{
    return video_path() / su::int_to_string(id);
}

std::filesystem::path Database::thumbnail_path() const
{
    return base_path() / "thumbnails";
}

std::filesystem::path Database::thumbnail_path(int id) const
{
    return thumbnail_path() / su::int_to_string(id);
}

inline int database::file_size(const std::string& path)
{
    std::ifstream file(path, std::ios::in | std::ios::binary | std::ios::ate);
    return static_cast<int>(file.tellg());
}

inline std::string database::read_file(const std::string& path, std::size_t offset, std::size_t length)
{
    std::ifstream file(path, std::ios::in | std::ios::binary);
    file.seekg(static_cast<std::streamoff>(offset));

    std::string file_content;
    file_content.resize_and_overwrite(length, [&file](char* buffer, std::size_t buffer_size) -> std::size_t {
        file.read(buffer, static_cast<std::streamoff>(buffer_size));
        return file.gcount();
    });
    return file_content;
}

inline std::string database::read_file(const std::string& path)
{
    // https://insanecoding.blogspot.com/2011/11/how-to-read-in-file-in-c.html
    std::ifstream file(path, std::ios::in | std::ios::binary | std::ios::ate);
    const std::size_t file_length{ static_cast<std::size_t>(file.tellg()) };
    return read_file(path, 0, file_length);
}

inline void database::write_file(const std::string& path, const std::string& content)
{
    std::ofstream file(path, std::ios::out | std::ios::binary | std::ios::trunc);
    file.write(content.data(), static_cast<std::streamoff>(content.size()));
}
