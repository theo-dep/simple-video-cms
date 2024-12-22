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
#include <array>
#include <fstream>

using namespace sqlite_orm;

namespace database
{
    auto storage(const std::filesystem::path& path) noexcept;
}

inline auto database::storage(const std::filesystem::path& path) noexcept
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

namespace database
{
    using StorageType = decltype(database::storage({}));

    template <typename Function>
    auto safe(const Function& callback, const std::source_location& location = std::source_location::current()) noexcept -> decltype(callback());

    int file_size(const std::string& path) noexcept;
    bool read_file(const std::string& path, std::size_t offset, const std::function<bool(const char*, std::size_t)>& callback) noexcept;
    std::string read_file(const std::string& path) noexcept;
    void write_file(const std::string& path, const std::string& content) noexcept;
}

Database::Database(const std::filesystem::path& path) noexcept
    : path_{ path }
{
}

bool Database::create_tables() const noexcept
{
    database::StorageType storage{ database::storage(path_) };
    return database::safe([&] {
        storage.sync_schema();
        return true;
    });
}

std::vector<int> Database::admin_video_list() const noexcept
{
    database::StorageType storage{ database::storage(path_) };
    return database::safe([&] {
        return storage.select(&Video::id);
    });
}

std::vector<int> Database::user_video_list(int user_id) const noexcept
{
    database::StorageType storage{ database::storage(path_) };
    return database::safe([&] {
        return storage.select(distinct(&VideoRight::video_id), where(c(&VideoRight::user_id) == user_id));
    });
}

std::vector<int> Database::no_user_video_list() const noexcept
{
    database::StorageType storage{ database::storage(path_) };
    return database::safe([&] {
        return storage.select(distinct(&Video::id), where(not(in(&Video::id, select(&VideoRight::video_id)))));
    });
}

std::string Database::video_title(int id) const noexcept
{
    database::StorageType storage{ database::storage(path_) };
    constexpr std::source_location location{ std::source_location::current() };
    return database::safe([&] {
        const std::optional video_title{
            storage.get_optional<Video>(id)
                .transform([](const Video& video) noexcept -> std::string {
                    return video.title;
                })
                .or_else([&] noexcept -> std::optional<std::string> {
                    logging::error<int const&>{ R"(Fail to fetch video title "{}")", id, location };
                    return {};
                })
        };
        return *video_title;
    });
}

int Database::video_views(int id) const noexcept
{
    database::StorageType storage{ database::storage(path_) };
    constexpr std::source_location location{ std::source_location::current() };
    return database::safe([&] {
        const std::optional video_views{
            storage.get_optional<Video>(id)
                .transform([](const Video& video) noexcept -> int {
                    return video.views;
                })
                .or_else([&] noexcept -> std::optional<int> {
                    logging::error<int const&>{ R"(Fail to fetch video views "{}")", id, location };
                    return -1;
                })
        };
        return *video_views;
    });
}

int Database::video_size(int id) const noexcept
{
    database::StorageType storage{ database::storage(path_) };
    constexpr std::source_location location{ std::source_location::current() };
    return database::safe([&] {
        const std::optional video_size{
            storage.get_optional<Video>(id)
                .transform([](const Video& video) noexcept -> int {
                    return database::file_size(video.content_path);
                })
                .or_else([&] noexcept -> std::optional<int> {
                    logging::error<int const&>{ R"(Fail to fetch video size "{}")", id, location };
                    return -1;
                })
        };
        return *video_size;
    });
}

// maybe https://www.sqlite.org/fasterthanfs.html
// https://github.com/fnc12/sqlite_orm/blob/v1.9/examples/blob_binding.cpp
// https://github.com/fnc12/sqlite_orm/blob/v1.9/examples/key_value.cpp

bool Database::video(int id, std::size_t offset, const std::function<bool(const char*, std::size_t)>& callback) const noexcept
{
    database::StorageType storage{ database::storage(path_) };
    constexpr std::source_location location{ std::source_location::current() };
    return database::safe([&] {
        const std::optional success{
            storage.get_optional<Video>(id)
                .transform([&offset, &callback](const Video& video) noexcept -> bool {
                    return database::read_file(video.content_path, offset, callback);
                })
                .or_else([&] noexcept -> std::optional<bool> {
                    logging::error<int const&>{ R"(Fail to fetch video content "{}")", id, location };
                    return false;
                })
        };
        return *success;
    });
}

std::string Database::thumbnail(int id) const noexcept
{
    database::StorageType storage{ database::storage(path_) };
    constexpr std::source_location location{ std::source_location::current() };
    return database::safe([&] {
        const std::optional thumbnail{
            storage.get_optional<Video>(id)
                .transform([](const Video& video) noexcept -> std::string {
                    return database::read_file(video.thumbnail_path);
                })
                .or_else([&] noexcept -> std::optional<std::string> {
                    logging::error<int const&>{ R"(Fail to fetch video thumbnail "{}")", id, location };
                    return std::string{};
                })
        };
        return *thumbnail;
    });
}

std::optional<int> Database::add_super_admin(const std::string& name, const std::string& password, const std::string& salt) const noexcept
{
    User user_super_admin{
        .name = name,
        .password = password,
        .salt = salt
    };

    database::StorageType storage{ database::storage(path_) };
    return database::safe([&] {
        user_super_admin.id = storage.insert(user_super_admin);

        const SuperAdmin super_admin{
            .id = user_super_admin.id
        };

        storage.replace(super_admin);
        return super_admin.id;
    });
}

bool Database::is_super_admin(int id) const noexcept
{
    database::StorageType storage{ database::storage(path_) };
    return database::safe([&] {
        return storage.get_all<SuperAdmin>(where(c(&SuperAdmin::id) == id)).size() == 1;
    });
}

bool Database::is_admin(int id) const noexcept
{
    database::StorageType storage{ database::storage(path_) };
    return database::safe([&] {
        return storage.get_all<User>(
                          where(
                              c(&User::id) == id and
                              (in(id, select(&Admin::id)) or
                               in(id, select(&SuperAdmin::id)))))
                   .size() == 1;
    });
}

bool Database::is_user(int id) const noexcept
{
    database::StorageType storage{ database::storage(path_) };
    return database::safe([&] {
        return storage.get_all<User>(
                          where(
                              c(&User::id) == id and
                              not(in(id, select(&Admin::id))) and
                              not(in(id, select(&SuperAdmin::id)))))
                   .size() == 1;
    });
}

std::optional<int> Database::add_admin(const std::string& name, const std::string& password, const std::string& salt) const noexcept
{
    User user_admin{
        .name = name,
        .password = password,
        .salt = salt
    };

    database::StorageType storage{ database::storage(path_) };
    return database::safe([&] {
        user_admin.id = storage.insert(user_admin);

        const Admin admin{
            .id = user_admin.id
        };

        storage.replace(admin);
        return admin.id;
    });
}

std::optional<int> Database::add_user(const std::string& name, const std::string& password, const std::string& salt) const noexcept
{
    User user{
        .name = name,
        .password = password,
        .salt = salt
    };

    database::StorageType storage{ database::storage(path_) };
    return database::safe([&] {
        user.id = storage.insert(user);
        return user.id;
    });
}

std::optional<int> Database::update_user_name(int id, const std::string& name) const noexcept
{
    database::StorageType storage{ database::storage(path_) };
    constexpr std::source_location location{ std::source_location::current() };
    return database::safe([&] {
        const std::optional user_id{
            storage.get_optional<User>(id)
                .and_then([&](User&& user) -> std::optional<int> {
                    user.name = name;
                    storage.update(std::move(user));
                    return id;
                })
                .or_else([&] noexcept -> std::optional<int> {
                    logging::error<int const&>{ R"(Fail to update user name "{}")", id, location };
                    return std::nullopt;
                })
        };
        return *user_id;
    });
}

std::optional<int> Database::update_user_password(int id, const std::string& password) const noexcept
{
    database::StorageType storage{ database::storage(path_) };
    constexpr std::source_location location{ std::source_location::current() };
    return database::safe([&] {
        const std::optional user_id{
            storage.get_optional<User>(id)
                .and_then([&](User&& user) -> std::optional<int> {
                    user.password = password;
                    storage.update(std::move(user));
                    return id;
                })
                .or_else([&] noexcept -> std::optional<int> {
                    logging::error<int const&>{ R"(Fail to update user password "{}")", id, location };
                    return std::nullopt;
                })
        };
        return *user_id;
    });
}

bool Database::delete_user(int id) const noexcept
{
    database::StorageType storage{ database::storage(path_) };
    constexpr std::source_location location{ std::source_location::current() };
    return database::safe([&] {
        const std::optional success{
            storage.get_optional<User>(id)
                .transform([&](const User& user) -> bool {
                    storage.remove<User>(user.id);
                    return true;
                })
                .or_else([&] noexcept -> std::optional<bool> {
                    logging::error<int const&>{ R"(Fail to delete user "{}")", id, location };
                    return false;
                })
        };
        return *success;
    });
}

int Database::user_id(const std::string& name) const noexcept
{
    database::StorageType storage{ database::storage(path_) };
    return database::safe([&] {
        const std::vector users{ storage.select(&User::id, where(c(&User::name) == name)) };
        return users.empty() ? -1 : users[0];
    });
}

std::string Database::user_name(int id) const noexcept
{
    database::StorageType storage{ database::storage(path_) };
    constexpr std::source_location location{ std::source_location::current() };
    return database::safe([&] {
        const std::optional user_name{
            storage.get_optional<User>(id)
                .transform([](const User& user) noexcept -> std::string {
                    return user.name;
                })
                .or_else([&] noexcept -> std::optional<std::string> {
                    logging::error<int const&>{ R"(Fail to fetch user name "{}")", id, location };
                    return {};
                })
        };
        return *user_name;
    });
}

std::string Database::user_password(int id) const noexcept
{
    database::StorageType storage{ database::storage(path_) };
    constexpr std::source_location location{ std::source_location::current() };
    return database::safe([&] {
        const std::optional user_password{
            storage.get_optional<User>(id)
                .transform([](const User& user) noexcept -> std::string {
                    return user.password;
                })
                .or_else([&] noexcept -> std::optional<std::string> {
                    logging::error<int const&>{ R"(Fail to fetch user password "{}")", id, location };
                    return {};
                })
        };
        return *user_password;
    });
}

std::string Database::user_salt(int id) const noexcept
{
    database::StorageType storage{ database::storage(path_) };
    constexpr std::source_location location{ std::source_location::current() };
    return database::safe([&] {
        const std::optional user_salt{
            storage.get_optional<User>(id)
                .transform([](const User& user) noexcept -> std::string {
                    return user.salt;
                })
                .or_else([&] noexcept -> std::optional<std::string> {
                    logging::error<int const&>{ R"(Fail to fetch user salt "{}")", id, location };
                    return {};
                })
        };
        return *user_salt;
    });
}

int Database::user_count() const noexcept
{
    database::StorageType storage{ database::storage(path_) };
    return database::safe([&] {
        return storage.count<User>();
    });
}

int Database::video_count() const noexcept
{
    database::StorageType storage{ database::storage(path_) };
    return database::safe([&] {
        return storage.count<Video>();
    });
}

int Database::view_count() const noexcept
{
    database::StorageType storage{ database::storage(path_) };
    return database::safe([&] {
        const std::unique_ptr sum{ storage.sum(&Video::views) };
        return sum ? *sum : 0;
    });
}

std::vector<int> Database::user_list() const noexcept
{
    database::StorageType storage{ database::storage(path_) };
    return database::safe([&] {
        return storage.select(except(select(&User::id), select(&Admin::id), select(&SuperAdmin::id)));
    });
}

std::vector<int> Database::admin_list() const noexcept
{
    database::StorageType storage{ database::storage(path_) };
    return database::safe([&] {
        return storage.select(union_all(select(&SuperAdmin::id), select(&Admin::id)));
    });
}

std::optional<int> Database::add_video(const std::string& title, const std::string& video_content) const noexcept
{
    const std::filesystem::path video_path{ path_.parent_path() / "videos" };
    if (!filesystem::create(video_path)) {
        return std::nullopt;
    }

    Video video{
        .title = title
    };

    database::StorageType storage{ database::storage(path_) };
    return database::safe([&] {
        video.id = storage.insert(video);

        video.content_path = video_path / su::int_to_string(video.id);
        storage.update(video);

        database::write_file(video.content_path, video_content);

        return video.id;
    });
}

std::optional<int> Database::add_video_thumbnail(int id, const std::string& thumbnail_content) const noexcept
{
    const std::filesystem::path thumbnail_path{ path_.parent_path() / "thumbnails" };
    if (!filesystem::create(thumbnail_path)) {
        return std::nullopt;
    }

    database::StorageType storage{ database::storage(path_) };
    return database::safe([&] {
        const std::optional video_id{
            storage.get_optional<Video>(id)
                .and_then([&](Video&& video) noexcept -> std::optional<Video> {
                    video.thumbnail_path = thumbnail_path / su::int_to_string(video.id);
                    storage.update(std::move(video));
                    return video;
                })
                .transform([&](const Video& video) noexcept -> int {
                    database::write_file(video.thumbnail_path, thumbnail_content);
                    return video.id;
                })
        };
        return video_id;
    });
}

bool Database::add_video_rights(int id, const std::vector<int>& user_ids) const noexcept
{
    std::vector<VideoRight> video_rights(user_ids.size());
    std::ranges::transform(user_ids, video_rights.begin(), [&id](int user_id) noexcept -> VideoRight {
        return VideoRight{ .video_id = id, .user_id = user_id };
    });

    database::StorageType storage{ database::storage(path_) };
    return database::safe([&] {
        storage.replace_range(video_rights.cbegin(), video_rights.cend());
        return true;
    });
}

std::optional<int> Database::update_video_title(int id, const std::string& title) const noexcept
{
    database::StorageType storage{ database::storage(path_) };
    constexpr std::source_location location{ std::source_location::current() };
    return database::safe([&] {
        const std::optional video_id{
            storage.get_optional<Video>(id)
                .and_then([&](Video&& video) -> std::optional<int> {
                    video.title = title;
                    storage.update(std::move(video));
                    return id;
                })
                .or_else([&] noexcept -> std::optional<int> {
                    logging::error<int const&>{ R"(Fail to update video title "{}")", id, location };
                    return std::nullopt;
                })
        };
        return *video_id;
    });
}

bool Database::update_video_rights(int id, const std::vector<int>& user_ids) const noexcept
{
    database::StorageType storage{ database::storage(path_) };
    return database::safe([&] {
        storage.remove_all<VideoRight>(where(c(&VideoRight::video_id) == id));
        return add_video_rights(id, user_ids);
    });
}

bool Database::delete_video(int id) const noexcept
{
    database::StorageType storage{ database::storage(path_) };
    constexpr std::source_location location{ std::source_location::current() };
    return database::safe([&] {
        const std::optional success{
            storage.get_optional<Video>(id)
                .and_then([](const Video& video) noexcept -> std::optional<Video> {
                    // remove video file
                    return filesystem::remove(video.content_path) ? std::optional(video) : std::nullopt;
                })
                .and_then([](const Video& video) noexcept -> std::optional<Video> {
                    // remove thumbnail file
                    return filesystem::remove(video.thumbnail_path) ? std::optional(video) : std::nullopt;
                })
                .transform([&](const Video& video) -> bool {
                    storage.remove<Video>(video.id);
                    return true;
                })
                .or_else([&] noexcept -> std::optional<bool> {
                    logging::error<int const&>{ R"(Fail to delete video "{}")", id, location };
                    return false;
                })
        };
        return *success;
    });
}

bool Database::increment_video_views(int id) const noexcept
{
    database::StorageType storage{ database::storage(path_) };
    constexpr std::source_location location{ std::source_location::current() };
    return database::safe([&] {
        const std::optional success{
            storage.get_optional<Video>(id)
                .transform([&](Video&& video) -> bool {
                    video.views += 1;
                    storage.update(std::move(video));
                    return true;
                })
                .or_else([&] noexcept -> std::optional<bool> {
                    logging::error<int const&>{ R"(Fail to increment video views "{}")", id, location };
                    return false;
                })
        };
        return *success;
    });
}

bool Database::has_video_right(int id) const noexcept
{
    database::StorageType storage{ database::storage(path_) };
    return database::safe([&] {
        return storage.get_all<VideoRight>(where(c(&VideoRight::video_id) == id)).size() == 0;
    });
}

bool Database::has_video_right(int id, int user_id) const noexcept
{
    database::StorageType storage{ database::storage(path_) };
    return database::safe([&] {
        return storage.get_all<VideoRight>(where(c(&VideoRight::video_id) == id and c(&VideoRight::user_id) == user_id)).size() > 0;
    });
}

std::vector<int> Database::video_right_list(int id) const noexcept
{
    database::StorageType storage{ database::storage(path_) };
    return database::safe([&] {
        return storage.select(distinct(&VideoRight::user_id), where(c(&VideoRight::video_id) == id));
    });
}

#ifdef DEBUG_LOG
template <typename T>
struct std::formatter<std::optional<T>, char> : std::formatter<std::string, char>
{
    template <class FmtContext>
    inline FmtContext::iterator format(const std::optional<T>& opt, FmtContext& ctx) const
    {
        if (opt)
            return std::formatter<std::string, char>::format(std::to_string(*opt), ctx);
        else
            return std::formatter<std::string, char>::format("invalid optional", ctx);
    }
};

template <typename T>
struct std::formatter<std::vector<T>, char> : std::formatter<std::string, char>
{
    template <class FmtContext>
    inline FmtContext::iterator format(const std::vector<T>& list, FmtContext& ctx) const
    {
        const std::string str{
            std::ranges::fold_left(
                list | std::views::transform([]<typename U>(const U& val) -> std::string { return std::to_string(val); }) | std::views::join_with(';'), std::string{}, std::plus{})
        };
        return std::formatter<std::string, char>::format(str, ctx);
    }
};
#endif

template <typename Function>
inline auto database::safe(const Function& callback, const std::source_location& location) noexcept -> decltype(callback())
{
    decltype(callback()) result{};
    try {
        result = callback();
    } catch (const std::exception& e) {
        logging::error<const char*>{ "sqlite_orm: {}", e.what(), location };
    } catch (...) {
        logging::error<std::string>{ "sqlite_orm: unknown error", location };
    }
    // logging::debug{ "sqlite_orm result: {} - {}", result, location.function_name() };
    return result;
}

inline int database::file_size(const std::string& path) noexcept
{
    std::ifstream file(path, std::ios::in | std::ios::binary | std::ios::ate);
    return file.tellg();
}

inline bool database::read_file(const std::string& path, std::size_t offset, const std::function<bool(const char*, std::size_t)>& callback) noexcept
{
    std::ifstream file(path, std::ios::in | std::ios::binary);
    file.seekg(offset);
    constexpr std::size_t chunk_size{ 4096 };
    std::array<char, chunk_size> buffer{};
    bool read_success{ true };
    while (file.good() && read_success) {
        file.read(buffer.data(), chunk_size);
        read_success = callback(buffer.data(), file.gcount());
    }
    return read_success && !file.bad();
}

inline std::string database::read_file(const std::string& path) noexcept
{
    // https://insanecoding.blogspot.com/2011/11/how-to-read-in-file-in-c.html
    std::ifstream file(path, std::ios::in | std::ios::binary | std::ios::ate);
    const std::streampos file_length{ file.tellg() };
    std::string file_content(file_length, '0');
    file.seekg(0);
    file.read(&file_content[0], file_length);
    return file_content;
}

inline void database::write_file(const std::string& path, const std::string& content) noexcept
{
    std::ofstream file(path, std::ios::out | std::ios::binary | std::ios::trunc);
    file.write(content.data(), content.size());
}
