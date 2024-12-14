#include "database.h"
#include "databaseschema.h"

#include "logging.h"
#include "stringutils.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#pragma GCC diagnostic ignored "-Wunused-but-set-parameter"
#include <sqlite_orm.h>
#pragma GCC diagnostic pop

#include <algorithm>
#include <array>
#include <fstream>

using namespace sqlite_orm;

namespace database
{
    auto storage(const std::filesystem::path& path) noexcept;

    // views is not yet implemented
    std::vector<int> admin_list(auto& storage) noexcept;

    int file_size(const std::string& path) noexcept;
    bool read_file(const std::string& path, const std::function<bool(const char*, std::size_t)>& callback) noexcept;
    std::string read_file(const std::string& path) noexcept;
    void write_file(const std::string& path, const std::string& content) noexcept;
}

inline auto database::storage(const std::filesystem::path& path) noexcept
{
    return make_storage(
        path,
        make_table("users",
                   make_column("id", &User::id, primary_key().autoincrement()),
                   make_column("name", &User::name),
                   make_column("password", &User::password),
                   make_column("salt", &User::salt)),
        make_table("super_admins",
                   make_column("id", &SuperAdmin::id, primary_key()),
                   foreign_key(&SuperAdmin::id).references(&User::id).on_delete.cascade()),
        make_table("admins",
                   make_column("id", &Admin::id, primary_key()),
                   foreign_key(&Admin::id).references(&User::id).on_delete.cascade()),
        make_table("videos",
                   make_column("id", &Video::id, primary_key().autoincrement()),
                   make_column("title", &Video::title),
                   make_column("views", &Video::views),
                   make_column("content_path", &Video::content_path),
                   make_column("thumbnail_path", &Video::thumbnail_path)),
        make_table("video_rights",
                   make_column("video_id", &VideoRight::video_id),
                   make_column("user_id", &VideoRight::user_id),
                   primary_key(&VideoRight::video_id, &VideoRight::user_id),
                   foreign_key(&VideoRight::video_id).references(&Video::id).on_delete.cascade(),
                   foreign_key(&VideoRight::user_id).references(&User::id).on_delete.cascade()));
    // make virtual table with FTS5 for search?
    // https://www.sqlite.org/fts5.html
}

Database::Database(const std::filesystem::path& path) noexcept
    : path_{ path }
{
}

bool Database::create_tables() const noexcept
{
    try {
        auto storage = database::storage(path_);
        storage.sync_schema();
        return true;
    } catch (const std::exception& e) {
        logging::error{ R"(Fail to create database at "{}": {})", path_.string(), e.what() };
        return false;
    }
}

std::vector<int> Database::video_list() const noexcept
{
    auto storage = database::storage(path_);
    return storage.select(&Video::id);
}

std::vector<int> Database::video_list(int user_id) const noexcept
{
    auto storage = database::storage(path_);
    return storage.select(&Video::id, where(c(&VideoRight::user_id) == user_id));
}

std::vector<int> Database::no_right_video_list() const noexcept
{
    auto storage = database::storage(path_);
    return storage.select(&Video::id, where(not(in(&Video::id, select(&VideoRight::video_id)))));
}

std::string Database::video_title(int id) const noexcept
{
    auto storage = database::storage(path_);
    const std::optional video{ storage.get_optional<Video>(id) };

    if (!video) {
        logging::error{ R"(Fail to fetch video title "{}")", id };
        return {};
    }

    return video->title;
}

int Database::video_views(int id) const noexcept
{
    auto storage = database::storage(path_);
    const std::optional video{ storage.get_optional<Video>(id) };

    if (!video) {
        logging::error{ R"(Fail to fetch video views "{}")", id };
        return -1;
    }

    return video->views;
}

int Database::video_size(int id) const noexcept
{
    auto storage = database::storage(path_);
    const std::optional video_size{
        storage.get_optional<Video>(id)
            .transform([](const Video& video) noexcept -> int {
                return database::file_size(video.content_path);
            })
    };

    if (!video_size) {
        logging::error{ R"(Fail to fetch video size "{}")", id };
        return -1;
    }

    return *video_size;
}

// maybe https://www.sqlite.org/fasterthanfs.html
// https://github.com/fnc12/sqlite_orm/blob/v1.9/examples/blob_binding.cpp
// https://github.com/fnc12/sqlite_orm/blob/v1.9/examples/key_value.cpp

bool Database::video(int id, const std::function<bool(const char*, std::size_t)>& callback) const noexcept
{
    auto storage = database::storage(path_);
    const std::optional success{
        storage.get_optional<Video>(id)
            .transform([&callback](const Video& video) noexcept -> bool {
                return database::read_file(video.content_path, callback);
            })
    };

    if (!success) {
        logging::error{ R"(Fail to fetch video content "{}")", id };
        return false;
    }

    return *success;
}

std::string Database::thumbnail(int id) const noexcept
{
    auto storage = database::storage(path_);
    const std::optional thumbnail{
        storage.get_optional<Video>(id)
            .transform([](const Video& video) noexcept -> std::string {
                return database::read_file(video.thumbnail_path);
            })
    };

    if (!thumbnail) {
        logging::error{ R"(Fail to fetch video thumbnail "{}")", id };
        return {};
    }

    return *thumbnail;
}

std::optional<int> Database::add_super_admin(const std::string& name, const std::string& password, const std::string& salt) const noexcept
{
    auto storage = database::storage(path_);

    User user_super_admin{
        .name = name,
        .password = password,
        .salt = salt
    };
    user_super_admin.id = storage.insert(user_super_admin);

    const SuperAdmin super_admin{
        .id = user_super_admin.id
    };

    storage.replace(super_admin);

    return super_admin.id;
}

bool Database::is_super_admin(int id) const noexcept
{
    auto storage = database::storage(path_);
    return storage.get_optional<SuperAdmin>(id).has_value();
}

bool Database::is_admin(int id) const noexcept
{
    auto storage = database::storage(path_);
    return storage.get_optional<Admin>(id).has_value();
}

bool Database::is_user(int id) const noexcept
{
    auto storage = database::storage(path_);
    return storage.get_all<User>(where(c(&User::id) == id and not(in(&User::id, database::admin_list(storage))))).size() == 1;
}

std::optional<int> Database::add_admin(const std::string& name, const std::string& password, const std::string& salt) const noexcept
{
    auto storage = database::storage(path_);

    User user_admin{
        .name = name,
        .password = password,
        .salt = salt
    };
    user_admin.id = storage.insert(user_admin);

    const Admin admin{
        .id = user_admin.id
    };

    storage.replace(admin);

    return admin.id;
}

std::optional<int> Database::add_user(const std::string& name, const std::string& password, const std::string& salt) const noexcept
{
    auto storage = database::storage(path_);

    User user{
        .name = name,
        .password = password,
        .salt = salt
    };
    user.id = storage.insert(user);

    return user.id;
}

bool Database::update_user_name(int id, const std::string& name) const noexcept
{
    auto storage = database::storage(path_);
    std::optional user{ storage.get_optional<User>(id) };

    if (!user) {
        logging::error{ R"(Fail to update user name "{}")", id };
        return false;
    }

    user->name = name;

    storage.update(*user);
    return true;
}

bool Database::update_user_password(int id, const std::string& password) const noexcept
{
    auto storage = database::storage(path_);
    std::optional user{ storage.get_optional<User>(id) };

    if (!user) {
        logging::error{ R"(Fail to update user password "{}")", id };
        return false;
    }

    user->password = password;

    storage.update(*user);
    return true;
}

bool Database::delete_user(int id) const noexcept
{
    auto storage = database::storage(path_);
    const std::optional user{ storage.get_optional<User>(id) };

    if (!user) {
        logging::error{ R"(Fail to delete user "{}")", id };
        return false;
    }

    storage.remove<User>(user->id);
    return true;
}

int Database::user_id(const std::string& name) const noexcept
{
    auto storage = database::storage(path_);
    return storage.select(&User::id, where(c(&User::name) == name))[0];
}

std::string Database::user_name(int id) const noexcept
{
    auto storage = database::storage(path_);
    const std::optional user{ storage.get_optional<User>(id) };

    if (!user) {
        logging::error{ R"(Fail to fetch user name "{}")", id };
        return {};
    }

    return user->name;
}

std::string Database::user_password(int id) const noexcept
{
    auto storage = database::storage(path_);
    const std::optional user{ storage.get_optional<User>(id) };

    if (!user) {
        logging::error{ R"(Fail to fetch user password "{}")", id };
        return {};
    }

    return user->password;
}

std::string Database::user_salt(int id) const noexcept
{
    auto storage = database::storage(path_);
    const std::optional user{ storage.get_optional<User>(id) };

    if (!user) {
        logging::error{ R"(Fail to fetch user salt "{}")", id };
        return {};
    }

    return user->salt;
}

int Database::user_count() const noexcept
{
    auto storage = database::storage(path_);
    return storage.count<User>();
}

int Database::video_count() const noexcept
{
    auto storage = database::storage(path_);
    return storage.count<Video>();
}

int Database::view_count() const noexcept
{
    auto storage = database::storage(path_);
    const std::unique_ptr sum{ storage.sum(&Video::views) };
    return sum ? *sum : 0;
}

std::vector<int> Database::user_list() const noexcept
{
    auto storage = database::storage(path_);
    return storage.select(&User::id, where(not(in(&User::id, database::admin_list(storage)))));
}

std::vector<int> Database::admin_list() const noexcept
{
    auto storage = database::storage(path_);
    return database::admin_list(storage);
}

std::optional<int> Database::add_video(const std::string& title, const std::string& video_content) const noexcept
{
    auto storage = database::storage(path_);

    const std::filesystem::path video_path{ path_.parent_path() / "videos" };

    std::error_code error_code;
    if (!std::filesystem::exists(video_path) && (!std::filesystem::create_directories(video_path, error_code) || error_code)) {
        logging::error{ R"(Fail to create "{}" directories: {} ({}))", video_path.string(), error_code.message(), error_code.value() };
        return std::nullopt;
    }

    Video video{
        .title = title
    };
    video.id = storage.insert(video);

    video.content_path = video_path / su::int_to_string(video.id);
    storage.replace(video);

    database::write_file(video.content_path, video_content);

    return video.id;
}

std::optional<int> Database::add_video_thumbnail(int id, const std::string& thumbnail_content) const noexcept
{
    auto storage = database::storage(path_);

    const std::filesystem::path thumbnail_path{ path_.parent_path() / "thumbnails" };

    std::error_code error_code;
    if (!std::filesystem::exists(thumbnail_path) && (!std::filesystem::create_directories(thumbnail_path, error_code) || error_code)) {
        logging::error{ R"(Fail to create "{}" directories: {} ({}))", thumbnail_path.string(), error_code.message(), error_code.value() };
        return std::nullopt;
    }

    const std::optional video_id{
        storage.get_optional<Video>(id)
            .and_then([&](const Video& video) noexcept -> std::optional<Video> {
                Video new_video{ video };
                new_video.thumbnail_path = thumbnail_path / su::int_to_string(video.id);
                storage.replace(new_video);
                return new_video;
            })
            .transform([&](const Video& video) noexcept -> int {
                database::write_file(video.thumbnail_path, thumbnail_content);
                return video.id;
            })
    };

    return video_id;
}

bool Database::add_video_rights(int id, const std::vector<int>& user_ids) const noexcept
{
    auto storage = database::storage(path_);

    std::vector<VideoRight> video_rights(user_ids.size());
    std::ranges::transform(user_ids, video_rights.begin(), [&id](int user_id) noexcept -> VideoRight {
        return VideoRight{ .video_id = id, .user_id = user_id };
    });

    storage.insert_range(video_rights.cbegin(), video_rights.cend());
    return true;
}

std::optional<int> Database::update_video_title(int id, const std::string& title) const noexcept
{
    auto storage = database::storage(path_);
    std::optional video{ storage.get_optional<Video>(id) };

    if (!video) {
        logging::error{ R"(Fail to update video title "{}")", id };
        return std::nullopt;
    }

    video->title = title;

    storage.update(*video);
    return id;
}

bool Database::update_video_rights(int id, const std::vector<int>& user_ids) const noexcept
{
    auto storage = database::storage(path_);
    storage.remove_all<VideoRight>(where(c(&VideoRight::video_id) == id));
    return add_video_rights(id, user_ids);
}

bool Database::delete_video(int id) const noexcept
{
    auto storage = database::storage(path_);
    const std::optional video{ storage.get_optional<Video>(id) };

    if (!video) {
        logging::error{ R"(Fail to delete video "{}")", id };
        return false;
    }

    std::error_code error_code;

    // remove video file
    if (!std::filesystem::remove(video->content_path, error_code) || error_code) {
        logging::error{ R"(Fail to remove "{}" with error {}: "{}")", video->content_path, error_code.value(), error_code.message() };
        return false;
    }

    // remove thumbnail file
    if (!std::filesystem::remove(video->thumbnail_path, error_code) || error_code) {
        logging::error{ R"(Fail to remove "{}" with error {}: "{}")", video->thumbnail_path, error_code.value(), error_code.message() };
        return false;
    }

    storage.remove<Video>(video->id);
    return true;
}

bool Database::increment_video_views(int id) const noexcept
{
    auto storage = database::storage(path_);
    std::optional video{ storage.get_optional<Video>(id) };

    if (!video) {
        logging::error{ R"(Fail to increment video views "{}")", id };
        return false;
    }

    video->views += 1;
    storage.replace(*video);
    return true;
}

bool Database::has_video_right(int id) const noexcept
{
    auto storage = database::storage(path_);
    return storage.count<VideoRight>(where(c(&VideoRight::video_id) == id)) > 0;
}

bool Database::has_video_right(int id, int user_id) const noexcept
{
    auto storage = database::storage(path_);
    return storage.count<VideoRight>(where(c(&VideoRight::video_id) == id and c(&VideoRight::user_id) == user_id)) > 0;
}

std::vector<int> Database::video_right_list(int id) const noexcept
{
    auto storage = database::storage(path_);
    return storage.select(&VideoRight::user_id, where(c(&VideoRight::video_id) == id));
}

inline std::vector<int> database::admin_list(auto& storage) noexcept
{
    std::vector admins{ storage.select(&SuperAdmin::id) };
    const std::vector append_admins{ storage.select(&Admin::id) };
#ifdef __cpp_lib_containers_ranges
    admins.append_range(append_admins);
#else
    admins.insert(admins.end(), append_admins.cbegin(), append_admins.cend());
#endif
    return admins;
}

inline int database::file_size(const std::string& path) noexcept
{
    std::ifstream file(path, std::ios::in | std::ios::binary);
    file.seekg(0, std::ios::end);
    return file.tellg();
}

inline bool database::read_file(const std::string& path, const std::function<bool(const char*, std::size_t)>& callback) noexcept
{
    std::ifstream file(path, std::ios::in | std::ios::binary);
    constexpr std::size_t chunk_size{ 4096 };
    std::array<char, chunk_size> buffer{};
    bool read_success{ true };
    while (file.read(buffer.data(), chunk_size) && file.gcount() > 0 && read_success) {
        read_success = callback(buffer.data(), file.gcount());
    }
    return read_success;
}

inline std::string database::read_file(const std::string& path) noexcept
{
    // https://insanecoding.blogspot.com/2011/11/how-to-read-in-file-in-c.html
    std::ifstream file(path, std::ios::in | std::ios::binary);
    file.seekg(0, std::ios::end);
    const std::streampos file_length{ file.tellg() };
    std::string file_content(file_length, '0');
    file.seekg(0, std::ios::beg);
    file.read(&file_content[0], file_length);
    return file_content;
}

inline void database::write_file(const std::string& path, const std::string& content) noexcept
{
    std::ofstream file(path, std::ios::out | std::ios::binary | std::ios::trunc);
    file.write(content.data(), content.size());
}
