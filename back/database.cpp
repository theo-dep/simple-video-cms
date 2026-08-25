#include "database.h"

#include "filesystem.h"
#include "logging.h"
#include "stringutils.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wuninitialized"
#pragma clang diagnostic ignored "-Wunused-local-typedefs"
#pragma clang diagnostic ignored "-Wunused-but-set-parameter"
#pragma clang diagnostic ignored "-Wunused-but-set-variable"
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#pragma clang diagnostic ignored "-Wdeprecated-literal-operator"
#pragma clang diagnostic ignored "-Wc++26-extensions"
#include <sqlite_orm/sqlite_orm.h>
#pragma clang diagnostic pop

#include <algorithm>
#include <mutex>

using namespace sqlite_orm;

namespace database
{
    auto storage(const std::filesystem::path& path);

    struct Version
    {
        int value;

        static constexpr auto table_name = "version";
    };
    static constexpr Version current_version{ .value = 3 };

    namespace v1
    {
        constexpr auto user_struct = struct_<User>(&User::id, &User::name, &User::password, &User::salt);
        constexpr auto video_struct = struct_<Video>(&Video::id, &Video::title);
    }
}

inline auto database::storage(const std::filesystem::path& path)
{
    return make_storage(
        path.string(),
        make_table(Version::table_name,
                   make_column("value", &Version::value)),
        make_table(::User::table_name,
                   make_column("id", &User::id, primary_key().autoincrement()),
                   make_column("name", &User::name, unique(), collate_nocase()),
                   make_column("password", &User::password),
                   make_column("salt", &User::salt),
                   make_column("deactivated", &User::deactivated)),
        make_table(::SuperAdmin::table_name,
                   make_column("id", &SuperAdmin::id, primary_key()),
                   foreign_key(&SuperAdmin::id).references(&User::id).on_delete.cascade()),
        make_table(::Admin::table_name,
                   make_column("id", &Admin::id, primary_key()),
                   foreign_key(&Admin::id).references(&User::id).on_delete.cascade()),
        make_table(::Group::table_name,
                   make_column("id", &Group::id, primary_key().autoincrement()),
                   make_column("name", &Group::name, unique(), collate_nocase())),
        make_table(::Video::table_name,
                   make_column("id", &Video::id, primary_key().autoincrement()),
                   make_column("title", &Video::title),
                   make_column("date", &Video::date),
                   make_column("location_id", &Video::location_id),
                   foreign_key(&Video::location_id).references(&Location::id).on_delete.set_null()),
        make_table(::GroupUser::table_name,
                   make_column("group_id", &GroupUser::group_id),
                   make_column("user_id", &GroupUser::user_id),
                   primary_key(&GroupUser::group_id, &GroupUser::user_id),
                   foreign_key(&GroupUser::group_id).references(&Group::id).on_delete.cascade(),
                   foreign_key(&GroupUser::user_id).references(&User::id).on_delete.cascade()),
        make_table(::VideoUserRight::table_name,
                   make_column("video_id", &VideoUserRight::video_id),
                   make_column("user_id", &VideoUserRight::user_id),
                   primary_key(&VideoUserRight::video_id, &VideoUserRight::user_id),
                   foreign_key(&VideoUserRight::video_id).references(&Video::id).on_delete.cascade(),
                   foreign_key(&VideoUserRight::user_id).references(&User::id).on_delete.cascade()),
        make_table(::VideoGroupRight::table_name,
                   make_column("video_id", &VideoGroupRight::video_id),
                   make_column("group_id", &VideoGroupRight::group_id),
                   primary_key(&VideoGroupRight::video_id, &VideoGroupRight::group_id),
                   foreign_key(&VideoGroupRight::video_id).references(&Video::id).on_delete.cascade(),
                   foreign_key(&VideoGroupRight::group_id).references(&Group::id).on_delete.cascade()),
        make_table(::UserVideoBookmark::table_name,
                   make_column("user_id", &UserVideoBookmark::user_id),
                   make_column("video_id", &UserVideoBookmark::video_id),
                   primary_key(&UserVideoBookmark::user_id, &UserVideoBookmark::video_id),
                   foreign_key(&UserVideoBookmark::user_id).references(&User::id).on_delete.cascade(),
                   foreign_key(&UserVideoBookmark::video_id).references(&Video::id).on_delete.cascade()),
        make_table(::Author::table_name,
                   make_column("id", &Author::id, primary_key().autoincrement()),
                   make_column("name", &Author::name, unique(), collate_nocase())),
        make_table(::VideoAuthor::table_name,
                   make_column("video_id", &VideoAuthor::video_id),
                   make_column("author_id", &VideoAuthor::author_id),
                   primary_key(&VideoAuthor::video_id, &VideoAuthor::author_id),
                   foreign_key(&VideoAuthor::video_id).references(&Video::id).on_delete.cascade(),
                   foreign_key(&VideoAuthor::author_id).references(&Author::id).on_delete.cascade()),
        make_table(::Location::table_name,
                   make_column("id", &Location::id, primary_key().autoincrement()),
                   make_column("name", &Location::name, unique(), collate_nocase())),
        make_table(::Tag::table_name,
                   make_column("id", &Tag::id, primary_key().autoincrement()),
                   make_column("name", &Tag::name, unique(), collate_nocase())),
        make_table(::VideoTag::table_name,
                   make_column("video_id", &VideoTag::video_id),
                   make_column("tag_id", &VideoTag::tag_id),
                   primary_key(&VideoTag::video_id, &VideoTag::tag_id),
                   foreign_key(&VideoTag::video_id).references(&Video::id).on_delete.cascade(),
                   foreign_key(&VideoTag::tag_id).references(&Tag::id).on_delete.cascade()),
        make_table(::SessionInfo::table_name,
                   make_column("id", &SessionInfo::id, primary_key()),
                   make_column("user_id", &SessionInfo::user_id),
                   make_column("creation_date", &SessionInfo::creation_date),
                   make_column("max_age_time", &SessionInfo::max_age_time),
                   foreign_key(&SessionInfo::user_id).references(&User::id).on_delete.cascade()));
    // make virtual table with FTS5 for search?
    // https://www.sqlite.org/fts5.html
}

namespace database
{
    using StorageType = decltype(database::storage({}));

    constexpr auto video_struct = struct_<Video>(&Video::id, &Video::title, &Video::date, &Video::location_id);
    constexpr auto location_struct = struct_<Location>(&Location::id, &Location::name);
    constexpr auto author_struct = struct_<Author>(&Author::id, &Author::name);
    constexpr auto tag_struct = struct_<Tag>(&Tag::id, &Tag::name);
    constexpr auto group_struct = struct_<Group>(&Group::id, &Group::name);
    constexpr auto user_struct = struct_<User>(&User::id, &User::name, &User::password, &User::salt);

    template <typename Table, typename Struct>
    std::vector<Table> backup(StorageType& storage, const Struct& db_struct, const std::vector<std::string>& table_names);

    template <typename Table>
    std::vector<Table> backup(StorageType& storage, const std::vector<std::string>& table_names);

    template <typename... Tables>
        requires(sizeof...(Tables) > 1)
    std::tuple<std::vector<Tables>...> backup(StorageType& storage, const std::vector<std::string>& table_names);

    template <typename... Tables>
    void drop_tables(StorageType& storage, const std::tuple<std::vector<Tables>...>& /*unused*/);

    template <typename... Tables>
    void restore_backup(StorageType& storage, const std::tuple<std::vector<Tables>...>& backup);
}

template <typename Table, typename Struct>
std::vector<Table> database::backup(StorageType& storage, const Struct& db_struct, const std::vector<std::string>& table_names)
{
    return std::ranges::contains(table_names, Table::table_name)
               ? storage.select(db_struct, from<Table>())
               : std::vector<Table>{};
}

template <typename Table>
inline std::vector<Table> database::backup(StorageType& storage, const std::vector<std::string>& table_names)
{
    return std::ranges::contains(table_names, Table::table_name)
               ? storage.get_all<Table>()
               : std::vector<Table>{};
}

template <typename... Tables>
    requires(sizeof...(Tables) > 1)
inline std::tuple<std::vector<Tables>...> database::backup(StorageType& storage, const std::vector<std::string>& table_names)
{
    return std::make_tuple(backup<Tables>(storage, table_names)...);
}

template <typename... Tables>
inline void database::drop_tables(StorageType& storage, const std::tuple<std::vector<Tables>...>& /*unused*/)
{
    // drop in reverse order to avoid foreign key constraint violations
    constexpr std::array table_names{ Tables::table_name... };
    for (const auto& name : table_names | std::views::reverse) {
        storage.drop_table_if_exists(name);
    }
}

template <typename... Tables>
inline void database::restore_backup(StorageType& storage, const std::tuple<std::vector<Tables>...>& backup)
{
    std::apply(
        [&storage](const auto&... vecs) {
            (..., [&storage](const auto& vec) {
                for (const auto& item : vec) {
                    storage.replace(item);
                }
            }(vecs));
        },
        backup);
}

Database::Database(std::filesystem::path path)
    : _path{ std::move(path) }
{
}

bool Database::create_tables() const
{
    const std::unique_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };

    const std::vector table_names{ storage.table_names() };

    const std::vector versions{ database::backup<database::Version>(storage, table_names) };
    if (table_names.empty() || (!versions.empty() && versions.back().value == database::current_version.value)) {
        storage.sync_schema();
        return true;
    }

    // start migration
    const int current_version{ versions.empty() ? 0 : versions.back().value };

    return storage.transaction([&] {
        try {
            logging::info{ "Migrating database from v{} to v3", current_version };

            const std::vector migrated_users{
                current_version <= 1 // to v2
                    ? database::backup<User>(storage, database::v1::user_struct, table_names)
                    : database::backup<User>(storage, table_names)
            };

            const std::vector migrated_videos{
                current_version <= 2 // to v3
                    ? database::backup<Video>(storage, database::v1::video_struct, table_names)
                    : database::backup<Video>(storage, table_names)
            };

            const std::tuple backup{
                std::tuple_cat(
                    std::make_tuple(migrated_users, migrated_videos),
                    database::backup<Group, Location, Author, Tag, SessionInfo,
                                     SuperAdmin, Admin, GroupUser, UserVideoBookmark,
                                     VideoUserRight, VideoGroupRight, VideoAuthor, VideoTag>(storage, table_names))
            };

            database::drop_tables(storage, backup);

            storage.sync_schema();

            database::restore_backup(storage, backup);

            storage.replace(database::current_version);

            logging::info{ "Migration done" };
        } catch (const std::exception& e) {
            logging::error{ "Migration failed: {}", e.what() };
            throw;
        }

        return true;
    });
}

std::vector<Video> Database::admin_video_list() const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    return storage.select(
        distinct(database::video_struct), from<Video>(),
        order_by(&Video::title).asc());
}

std::vector<Video> Database::user_video_list(int user_id) const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    return storage.select(
        distinct(database::video_struct), from<Video>(),
        where(
            (not_in(&Video::id, select(&VideoUserRight::video_id)) and
             not_in(&Video::id, select(&VideoGroupRight::video_id))) or
            in(&Video::id, union_(
                               select(distinct(&VideoUserRight::video_id), where(c(&VideoUserRight::user_id) == user_id)),
                               select(
                                   distinct(&VideoGroupRight::video_id),
                                   where(
                                       in(&VideoGroupRight::group_id,
                                          select(distinct(&GroupUser::group_id), where(c(&GroupUser::user_id) == user_id)))))))),
        order_by(&Video::title).asc());
}

std::vector<Video> Database::no_user_video_list() const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    return storage.select(
        distinct(database::video_struct), from<Video>(),
        where(
            not_in(&Video::id, select(&VideoUserRight::video_id)) and
            not_in(&Video::id, select(&VideoGroupRight::video_id))),
        order_by(&Video::title).asc());
}

bool Database::bookmarked(int user_id, int video_id) const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    const std::optional bookmark{ storage.get_optional<UserVideoBookmark>(user_id, video_id) };
    // if the video is not bookmarked, the entry does not exist in the table
    return bookmark.has_value();
}

bool Database::set_bookmark(int user_id, int video_id, bool bookmarked) const
{
    const std::unique_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };

    const std::optional bookmark{ storage.get_optional<UserVideoBookmark>(user_id, video_id) };
    if ((bookmark && bookmarked) || (!bookmark && !bookmarked)) {
        logging::error{ R"(Fail to set bookmark "{}" for user "{}" and video "{}")", bookmarked, user_id, video_id };
        return false;
    }

    if (bookmarked) {
        const UserVideoBookmark bookmark{
            .user_id = user_id,
            .video_id = video_id
        };
        storage.replace(bookmark);
    } else {
        storage.remove<UserVideoBookmark>(user_id, video_id);
    }

    return true;
}

std::optional<Video> Database::video(int video_id) const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    return storage.get_optional<Video>(video_id);
}

int Database::video_size(int video_id) const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    const std::optional video_size{
        storage.get_optional<Video>(video_id)
            .transform([&](const Video& video) -> int {
                return filesystem::file_size(video_path(video.id));
            })
            .or_else([&] -> std::optional<int> {
                logging::error{ R"(Fail to fetch video size "{}")", video_id };
                return -1;
            })
    };
    return video_size.value_or(-1);
}

// maybe https://www.sqlite.org/fasterthanfs.html
// https://github.com/fnc12/sqlite_orm/blob/v1.9/examples/blob_binding.cpp
// https://github.com/fnc12/sqlite_orm/blob/v1.9/examples/key_value.cpp

std::string Database::video(int video_id, std::size_t offset, std::size_t length) const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    const std::optional video{
        storage.get_optional<Video>(video_id)
            .transform([&](const Video& video) -> std::string {
                return filesystem::read_file(video_path(video.id), offset, length);
            })
            .or_else([&] -> std::optional<std::string> {
                logging::error{ R"(Fail to fetch video content "{}")", video_id };
                return std::string{};
            })
    };
    return video.value_or(std::string{});
}

std::string Database::video_playlist(int video_id) const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    const std::optional playlist{
        storage.get_optional<Video>(video_id)
            .transform([&](const Video& video) -> std::string {
                const std::filesystem::path path{ hls_video_path(video.id) };
                const std::filesystem::path playlist_path{ path / (hls_video_name(video_id) + ".m3u8") };
                return filesystem::read_file(playlist_path);
            })
            .or_else([&] -> std::optional<std::string> {
                logging::error{ R"(Fail to fetch video playlist "{}")", video_id };
                return std::string{};
            })
    };
    return playlist.value_or(std::string{});
}

std::string Database::video_segment(int video_id, const std::string& segment) const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    const std::optional segment_content{
        storage.get_optional<Video>(video_id)
            .transform([&](const Video& video) -> std::string {
                const std::filesystem::path path{ hls_video_path(video.id) };
                const std::filesystem::path segment_path{ path / segment };
                return filesystem::read_file(segment_path);
            })
            .or_else([&] -> std::optional<std::string> {
                logging::error{ R"(Fail to fetch video segment "{}": "{}")", video_id, segment };
                return std::string{};
            })
    };
    return segment_content.value_or(std::string{});
}

std::string Database::thumbnail(int video_id) const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    const std::optional thumbnail{
        storage.get_optional<Video>(video_id)
            .transform([&](const Video& video) -> std::string {
                return filesystem::read_file(thumbnail_path(video.id));
            })
            .or_else([&] -> std::optional<std::string> {
                logging::error{ R"(Fail to fetch video thumbnail "{}")", video_id };
                return std::string{};
            })
    };
    return thumbnail.value_or(std::string{});
}

std::optional<int> Database::add_super_admin(const std::string& name, const std::string& salt) const
{
    User user_super_admin{
        .name = name,
        .salt = salt
    };

    const std::unique_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    user_super_admin.id = storage.insert(user_super_admin);

    const SuperAdmin super_admin{
        .id = user_super_admin.id
    };

    storage.replace(super_admin);
    return super_admin.id;
}

bool Database::is_super_admin(int user_id) const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    return storage.get_all<SuperAdmin>(where(c(&SuperAdmin::id) == user_id)).size() == 1;
}

bool Database::is_admin(int user_id) const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    return storage.get_all<User>(
                      where(
                          c(&User::id) == user_id and
                          (in(user_id, select(&Admin::id)) or
                           in(user_id, select(&SuperAdmin::id)))))
               .size() == 1;
}

std::optional<int> Database::add_admin(const std::string& name, const std::string& salt) const
{
    User user_admin{
        .name = name,
        .salt = salt
    };

    const std::unique_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    user_admin.id = storage.insert(user_admin);

    const Admin admin{
        .id = user_admin.id
    };

    storage.replace(admin);
    return admin.id;
}

std::optional<int> Database::add_user(const std::string& name, const std::string& salt) const
{
    User user{
        .name = name,
        .salt = salt
    };

    const std::unique_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    user.id = storage.insert(user);
    return user.id;
}

std::optional<int> Database::add_password(int user_id, const std::string& password) const
{
    const std::unique_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    const std::optional id{
        storage.get_optional<User>(user_id)
            .and_then([&](User user) -> std::optional<int> {
                user.password = password;
                storage.update(user);
                return user_id;
            })
            .or_else([&] -> std::optional<int> {
                logging::error{ R"(Fail to add password "{}")", user_id };
                return std::nullopt;
            })
    };
    return id;
}

std::optional<int> Database::update_username(int user_id, const std::string& name) const
{
    const std::unique_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    const std::optional id{
        storage.get_optional<User>(user_id)
            .and_then([&](User user) -> std::optional<int> {
                user.name = name;
                storage.update(user);
                return user_id;
            })
            .or_else([&] -> std::optional<int> {
                logging::error{ R"(Fail to update username "{}")", user_id };
                return std::nullopt;
            })
    };
    return id;
}

std::optional<int> Database::update_password(int user_id, const std::string& password) const
{
    const std::unique_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    const std::optional id{
        storage.get_optional<User>(user_id)
            .and_then([&](User user) -> std::optional<int> {
                user.password = password;
                storage.update(user);
                return user_id;
            })
            .or_else([&] -> std::optional<int> {
                logging::error{ R"(Fail to update password "{}")", user_id };
                return std::nullopt;
            })
    };
    return id;
}

std::optional<int> Database::clear_password(int user_id) const
{
    const std::unique_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    const std::optional id{
        storage.get_optional<User>(user_id)
            .and_then([&](User user) -> std::optional<int> {
                user.password.reset();
                storage.update(user);
                return user_id;
            })
            .or_else([&] -> std::optional<int> {
                logging::error{ R"(Fail to clear password "{}")", user_id };
                return std::nullopt;
            })
    };
    return id;
}

std::optional<int> Database::deactivate_user(int user_id, bool deactivated) const
{
    const std::unique_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    const std::optional id{
        storage.get_optional<User>(user_id)
            .and_then([&](User user) -> std::optional<int> {
                user.deactivated = deactivated;
                storage.update(user);
                return user_id;
            })
            .or_else([&] -> std::optional<int> {
                logging::error{ R"(Fail to deactivate "{}")", user_id };
                return std::nullopt;
            })
    };
    return id;
}

bool Database::delete_user(int user_id) const
{
    const std::unique_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    const std::optional success{
        storage.get_optional<User>(user_id)
            .transform([&](const User& user) -> bool {
                storage.remove<User>(user.id);
                return true;
            })
            .or_else([&] -> std::optional<bool> {
                logging::error{ R"(Fail to delete user "{}")", user_id };
                return false;
            })
    };
    return success.value_or(false);
}

bool Database::user_exists(const std::string& name) const
{
    return user(name).has_value();
}

bool Database::user_exists(int user_id, const std::string& name) const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    const std::vector users{ storage.select(&User::id, where(c(&User::name) == name and c(&User::id) != user_id)) };
    return users.size() == 1;
}

std::optional<User> Database::user(const std::string& name) const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    const std::vector users{ storage.select(database::user_struct, where(c(&User::name) == name)) };
    return users.size() == 1 ? std::optional(users.at(0)) : std::nullopt;
}

std::optional<User> Database::user(int user_id) const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    return storage.get_optional<User>(user_id);
}

bool Database::deactivated_user(int user_id) const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    const std::optional deactivated_user{
        storage.get_optional<User>(user_id)
            .transform([](const User& user) -> bool {
                return user.deactivated;
            })
            .or_else([&] -> std::optional<bool> {
                logging::error{ R"(Fail to fetch deactivated user "{}")", user_id };
                return true;
            })
    };
    // true on error to block the connection
    return deactivated_user.value_or(true);
}

int Database::user_count() const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    return storage.count<User>();
}

int Database::group_count() const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    return storage.count<Group>();
}

int Database::video_count() const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    return storage.count<Video>();
}

std::vector<User> Database::user_list() const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    return storage.select(
        distinct(database::user_struct), from<User>(),
        where(
            not_in(&User::id, select(&Admin::id)) and
            not_in(&User::id, select(&SuperAdmin::id))),
        order_by(&User::name).asc());
}

std::vector<User> Database::admin_list() const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    return storage.select(
        distinct(database::user_struct), from<User>(),
        where(in(&User::id, union_(select(&Admin::id), select(&SuperAdmin::id)))),
        order_by(&User::name).asc());
}

std::vector<Group> Database::group_list() const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    return storage.select(
        distinct(database::group_struct), from<Group>(),
        order_by(&Group::name).asc());
}

bool Database::group_exists(const std::string& name) const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    const std::vector groups{ storage.select(&Group::id, where(c(&Group::name) == name)) };
    return groups.size() == 1;
}

bool Database::group_exists(int group_id, const std::string& name) const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    const std::vector groups{ storage.select(&Group::id, where(c(&Group::name) == name and c(&Group::id) != group_id)) };
    return groups.size() == 1;
}

std::optional<Group> Database::group(int group_id) const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    return storage.get_optional<Group>(group_id);
}

std::optional<int> Database::add_group(const std::string& name) const
{
    Group group{
        .name = name,
    };

    const std::unique_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    group.id = storage.insert(group);
    return group.id;
}

bool Database::add_group_users(int group_id, const std::vector<int>& user_ids) const
{
    std::vector<GroupUser> group_users(user_ids.size());
    std::ranges::transform(user_ids, group_users.begin(), [&group_id](int user_id) -> GroupUser {
        return GroupUser{ .group_id = group_id, .user_id = user_id };
    });

    const std::unique_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    storage.replace_range(group_users.cbegin(), group_users.cend());
    return true;
}

bool Database::add_group_video_rights(int group_id, const std::vector<int>& video_ids) const
{
    std::vector<VideoGroupRight> video_rights(video_ids.size());
    std::ranges::transform(video_ids, video_rights.begin(), [&group_id](int video_id) -> VideoGroupRight {
        return VideoGroupRight{ .video_id = video_id, .group_id = group_id };
    });

    const std::unique_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    storage.replace_range(video_rights.cbegin(), video_rights.cend());
    return true;
}

bool Database::add_user_groups(int user_id, const std::vector<int>& group_ids) const
{
    std::vector<GroupUser> group_users(group_ids.size());
    std::ranges::transform(group_ids, group_users.begin(), [&user_id](int group_id) -> GroupUser {
        return GroupUser{ .group_id = group_id, .user_id = user_id };
    });

    const std::unique_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    storage.replace_range(group_users.cbegin(), group_users.cend());
    return true;
}

bool Database::add_user_video_rights(int user_id, const std::vector<int>& video_ids) const
{
    std::vector<VideoUserRight> video_rights(video_ids.size());
    std::ranges::transform(video_ids, video_rights.begin(), [&user_id](int video_id) -> VideoUserRight {
        return VideoUserRight{ .video_id = video_id, .user_id = user_id };
    });

    const std::unique_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    storage.replace_range(video_rights.cbegin(), video_rights.cend());
    return true;
}

std::optional<int> Database::update_group_name(int group_id, const std::string& name) const
{
    const std::unique_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    const std::optional id{
        storage.get_optional<Group>(group_id)
            .and_then([&](Group group) -> std::optional<int> {
                group.name = name;
                storage.update(group);
                return group_id;
            })
            .or_else([&] -> std::optional<int> {
                logging::error{ R"(Fail to update group name "{}")", group_id };
                return std::nullopt;
            })
    };
    return id;
}

bool Database::update_group_users(int group_id, const std::vector<int>& user_ids) const
{
    {
        const std::unique_lock lock(_mutex);
        database::StorageType storage{ database::storage(_path) };
        storage.remove_all<GroupUser>(where(c(&GroupUser::group_id) == group_id));
    }
    return add_group_users(group_id, user_ids);
}

bool Database::update_group_video_rights(int group_id, const std::vector<int>& video_ids) const
{
    {
        const std::unique_lock lock(_mutex);
        database::StorageType storage{ database::storage(_path) };
        storage.remove_all<VideoGroupRight>(where(c(&VideoGroupRight::group_id) == group_id));
    }
    return add_group_video_rights(group_id, video_ids);
}

bool Database::update_user_groups(int user_id, const std::vector<int>& group_ids) const
{
    {
        const std::unique_lock lock(_mutex);
        database::StorageType storage{ database::storage(_path) };
        storage.remove_all<GroupUser>(where(c(&GroupUser::user_id) == user_id));
    }
    return add_user_groups(user_id, group_ids);
}

bool Database::update_user_video_rights(int user_id, const std::vector<int>& video_ids) const
{
    {
        const std::unique_lock lock(_mutex);
        database::StorageType storage{ database::storage(_path) };
        storage.remove_all<VideoUserRight>(where(c(&VideoUserRight::user_id) == user_id));
    }
    return add_user_video_rights(user_id, video_ids);
}

bool Database::delete_group(int group_id) const
{
    const std::unique_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    const std::optional success{
        storage.get_optional<Group>(group_id)
            .transform([&](const Group& group) -> bool {
                storage.remove<Group>(group.id);
                return true;
            })
            .or_else([&] -> std::optional<bool> {
                logging::error{ R"(Fail to delete group "{}")", group_id };
                return false;
            })
    };
    return success.value_or(false);
}

std::vector<User> Database::group_user_list(int group_id) const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    return storage.select(
        distinct(database::user_struct), from<User>(),
        where(
            in(&User::id, select(distinct(&GroupUser::user_id), where(c(&GroupUser::group_id) == group_id)))),
        order_by(&User::name).asc());
}

std::vector<Video> Database::group_video_list(int group_id) const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    return storage.select(
        distinct(database::video_struct), from<Video>(),
        where(
            in(&Video::id, select(distinct(&VideoGroupRight::video_id), where(c(&VideoGroupRight::group_id) == group_id)))),
        order_by(&Video::title).asc());
}

std::vector<Group> Database::user_group_list(int user_id) const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    return storage.select(
        distinct(database::group_struct), from<Group>(),
        where(
            in(&Group::id, select(distinct(&GroupUser::group_id), where(c(&GroupUser::user_id) == user_id)))),
        order_by(&Group::name).asc());
}

std::vector<Video> Database::unique_user_video_list(int user_id) const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    return storage.select(
        distinct(database::video_struct), from<Video>(),
        where(
            in(&Video::id, select(distinct(&VideoUserRight::video_id), where(c(&VideoUserRight::user_id) == user_id)))),
        order_by(&Video::title).asc());
}

std::optional<int> Database::add_video(const std::string& title, const std::optional<std::string>& date, const std::optional<int>& location_id, const std::string& video_content) const
{
    if (!filesystem::create(video_path())) {
        return std::nullopt;
    }

    Video video{
        .title = title,
        .date = date,
        .location_id = location_id
    };

    const std::unique_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    video.id = storage.insert(video);

    if (!filesystem::create(hls_video_path(video.id))) {
        return std::nullopt;
    }

    filesystem::write_file(video_path(video.id), video_content);

    return video.id;
}

std::optional<int> Database::add_video_thumbnail(int video_id, const std::string& thumbnail_content) const
{
    if (!filesystem::create(thumbnail_path())) {
        return std::nullopt;
    }

    const std::unique_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    const std::optional id{
        storage.get_optional<Video>(video_id)
            .transform([&](const Video& video) -> int {
                filesystem::write_file(thumbnail_path(video.id), thumbnail_content);
                return video.id;
            })
    };
    return id;
}

bool Database::add_video_group_rights(int video_id, const std::vector<int>& group_ids) const
{
    std::vector<VideoGroupRight> video_rights(group_ids.size());
    std::ranges::transform(group_ids, video_rights.begin(), [&video_id](int group_id) -> VideoGroupRight {
        return VideoGroupRight{ .video_id = video_id, .group_id = group_id };
    });

    const std::unique_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    storage.replace_range(video_rights.cbegin(), video_rights.cend());
    return true;
}

bool Database::add_video_user_rights(int video_id, const std::vector<int>& user_ids) const
{
    std::vector<VideoUserRight> video_rights(user_ids.size());
    std::ranges::transform(user_ids, video_rights.begin(), [&video_id](int user_id) -> VideoUserRight {
        return VideoUserRight{ .video_id = video_id, .user_id = user_id };
    });

    const std::unique_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    storage.replace_range(video_rights.cbegin(), video_rights.cend());
    return true;
}

std::optional<int> Database::update_video(int video_id, const std::string& title, const std::optional<std::string>& date, const std::optional<int>& location_id) const
{
    const std::unique_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    const std::optional id{
        storage.get_optional<Video>(video_id)
            .and_then([&](Video video) -> std::optional<int> {
                video.title = title;
                video.date = date;
                video.location_id = location_id;
                storage.update(video);
                return video_id;
            })
            .or_else([&] -> std::optional<int> {
                logging::error{ R"(Fail to update video title "{}")", video_id };
                return std::nullopt;
            })
    };
    return id;
}

bool Database::update_video_authors(int video_id, const std::vector<int>& author_ids) const
{
    {
        const std::unique_lock lock(_mutex);
        database::StorageType storage{ database::storage(_path) };
        storage.remove_all<VideoAuthor>(where(c(&VideoAuthor::video_id) == video_id));
    }
    return add_video_authors(video_id, author_ids);
}

bool Database::update_video_tags(int video_id, const std::vector<int>& tag_ids) const
{
    {
        const std::unique_lock lock(_mutex);
        database::StorageType storage{ database::storage(_path) };
        storage.remove_all<VideoTag>(where(c(&VideoTag::video_id) == video_id));
    }
    return add_video_tags(video_id, tag_ids);
}

bool Database::update_video_group_rights(int video_id, const std::vector<int>& group_ids) const
{
    {
        const std::unique_lock lock(_mutex);
        database::StorageType storage{ database::storage(_path) };
        storage.remove_all<VideoGroupRight>(where(c(&VideoGroupRight::video_id) == video_id));
    }
    return add_video_group_rights(video_id, group_ids);
}

bool Database::update_video_user_rights(int video_id, const std::vector<int>& user_ids) const
{
    {
        const std::unique_lock lock(_mutex);
        database::StorageType storage{ database::storage(_path) };
        storage.remove_all<VideoUserRight>(where(c(&VideoUserRight::video_id) == video_id));
    }
    return add_video_user_rights(video_id, user_ids);
}

bool Database::delete_video(int video_id) const
{
    const std::unique_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    const std::optional success{
        storage.get_optional<Video>(video_id)
            .and_then([&](const Video& video) -> std::optional<Video> {
                // remove video file
                return filesystem::remove(video_path(video.id)) ? std::optional(video) : std::nullopt;
            })
            .and_then([&](const Video& video) -> std::optional<Video> {
                // remove video directory
                return ::filesystem::remove_directory(hls_video_path(video.id)) ? std::optional(video) : std::nullopt;
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
                logging::error{ R"(Fail to delete video "{}")", video_id };
                return false;
            })
    };
    return success.value_or(false);
}

bool Database::has_video_right(int video_id) const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    return storage.get_all<VideoUserRight>(where(c(&VideoUserRight::video_id) == video_id)).empty() &&
           storage.get_all<VideoGroupRight>(where(c(&VideoGroupRight::video_id) == video_id)).empty();
}

bool Database::has_video_right(int video_id, int user_id) const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    return !storage.get_all<VideoUserRight>(where(
                                                c(&VideoUserRight::video_id) == video_id and
                                                c(&VideoUserRight::user_id) == user_id))
                .empty() ||
           !storage.get_all<VideoGroupRight>(where(c(&VideoGroupRight::video_id) == video_id and
                                                   in(&VideoGroupRight::group_id,
                                                      select(&GroupUser::group_id, where(c(&GroupUser::user_id) == user_id)))))
                .empty();
}

std::vector<Location> Database::location_list() const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    return storage.select(
        distinct(database::location_struct), from<Location>(),
        order_by(&Location::name).asc());
}

std::optional<Location> Database::location(int location_id) const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    return storage.get_optional<Location>(location_id);
}

bool Database::location_exists(const std::string& name) const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    const std::vector places{ storage.select(&Location::id, where(c(&Location::name) == name)) };
    return places.size() == 1;
}

std::optional<int> Database::add_location(const std::string& name) const
{
    Location location{
        .name = name
    };

    const std::unique_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    location.id = storage.insert(location);
    return location.id;
}

bool Database::update_location_name(int location_id, const std::string& name) const
{
    const std::unique_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    const std::optional success{
        storage.get_optional<Location>(location_id)
            .transform([&](Location location) -> bool {
                location.name = name;
                storage.update(location);
                return true;
            })
            .or_else([&] -> std::optional<bool> {
                logging::error{ R"(Fail to update location name "{}")", location_id };
                return false;
            })
    };
    return success.value_or(false);
}

bool Database::delete_location(int location_id) const
{
    const std::unique_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    const std::optional success{
        storage.get_optional<Location>(location_id)
            .transform([&](const Location& location) -> bool {
                storage.remove<Location>(location.id);
                return true;
            })
            .or_else([&] -> std::optional<bool> {
                logging::error{ R"(Fail to delete location "{}")", location_id };
                return false;
            })
    };
    return success.value_or(false);
}

std::vector<Author> Database::author_list() const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    return storage.select(
        distinct(database::author_struct), from<Author>(),
        order_by(&Author::name).asc());
}

bool Database::author_exists(const std::string& name) const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    const std::vector authors{ storage.select(&Author::id, where(c(&Author::name) == name)) };
    return authors.size() == 1;
}

std::optional<int> Database::add_author(const std::string& name) const
{
    Author author{
        .name = name
    };

    const std::unique_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    author.id = storage.insert(author);
    return author.id;
}

bool Database::update_author_name(int author_id, const std::string& name) const
{
    const std::unique_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    const std::optional success{
        storage.get_optional<Author>(author_id)
            .transform([&](Author author) -> bool {
                author.name = name;
                storage.update(author);
                return true;
            })
            .or_else([&] -> std::optional<bool> {
                logging::error{ R"(Fail to update author name "{}")", author_id };
                return false;
            })
    };
    return success.value_or(false);
}

bool Database::delete_author(int author_id) const
{
    const std::unique_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    const std::optional success{
        storage.get_optional<Author>(author_id)
            .transform([&](const Author& author) -> bool {
                storage.remove<Author>(author.id);
                return true;
            })
            .or_else([&] -> std::optional<bool> {
                logging::error{ R"(Fail to delete author "{}")", author_id };
                return false;
            })
    };
    return success.value_or(false);
}

std::vector<Tag> Database::tag_list() const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    return storage.select(
        distinct(database::tag_struct), from<Tag>(),
        order_by(&Tag::name).asc());
}

bool Database::tag_exists(const std::string& name) const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    const std::vector tags{ storage.select(&Tag::id, where(c(&Tag::name) == name)) };
    return tags.size() == 1;
}

std::optional<int> Database::add_tag(const std::string& name) const
{
    Tag tag{
        .name = name
    };

    const std::unique_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    tag.id = storage.insert(tag);
    return tag.id;
}

bool Database::update_tag_name(int tag_id, const std::string& name) const
{
    const std::unique_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    const std::optional success{
        storage.get_optional<Tag>(tag_id)
            .transform([&](Tag tag) -> bool {
                tag.name = name;
                storage.update(tag);
                return true;
            })
            .or_else([&] -> std::optional<bool> {
                logging::error{ R"(Fail to update tag name "{}")", tag_id };
                return false;
            })
    };
    return success.value_or(false);
}

bool Database::delete_tag(int tag_id) const
{
    const std::unique_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    const std::optional success{
        storage.get_optional<Tag>(tag_id)
            .transform([&](const Tag& t) -> bool {
                storage.remove<Tag>(t.id);
                return true;
            })
            .or_else([&] -> std::optional<bool> {
                logging::error{ R"(Fail to delete tag "{}")", tag_id };
                return false;
            })
    };
    return success.value_or(false);
}

bool Database::add_video_authors(int video_id, const std::vector<int>& author_ids) const
{
    std::vector<VideoAuthor> video_authors(author_ids.size());
    std::ranges::transform(author_ids, video_authors.begin(), [&video_id](int author_id) -> VideoAuthor {
        return VideoAuthor{ .video_id = video_id, .author_id = author_id };
    });

    const std::unique_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    storage.replace_range(video_authors.cbegin(), video_authors.cend());
    return true;
}

bool Database::add_video_tags(int video_id, const std::vector<int>& tag_ids) const
{
    std::vector<VideoTag> video_tags(tag_ids.size());
    std::ranges::transform(tag_ids, video_tags.begin(), [&video_id](int tag_id) -> VideoTag {
        return VideoTag{ .video_id = video_id, .tag_id = tag_id };
    });

    const std::unique_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    storage.replace_range(video_tags.cbegin(), video_tags.cend());
    return true;
}

std::vector<Author> Database::video_author_list(int video_id) const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    return storage.select(
        distinct(database::author_struct), from<Author>(),
        where(
            in(&Author::id, select(distinct(&VideoAuthor::author_id), where(c(&VideoAuthor::video_id) == video_id)))),
        order_by(&Author::name).asc());
}

std::vector<Tag> Database::video_tag_list(int video_id) const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    return storage.select(
        distinct(database::tag_struct), from<Tag>(),
        where(
            in(&Tag::id, select(distinct(&VideoTag::tag_id), where(c(&VideoTag::video_id) == video_id)))),
        order_by(&Tag::name).asc());
}

std::vector<Group> Database::video_group_right_list(int video_id) const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    return storage.select(
        distinct(database::group_struct), from<Group>(),
        where(
            in(&Group::id, select(distinct(&VideoGroupRight::group_id), where(c(&VideoGroupRight::video_id) == video_id)))),
        order_by(&Group::name).asc());
}

std::vector<User> Database::video_user_right_list(int video_id) const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    return storage.select(
        distinct(database::user_struct), from<User>(),
        where(
            in(&User::id, select(distinct(&VideoUserRight::user_id), where(c(&VideoUserRight::video_id) == video_id)))),
        order_by(&User::name).asc());
}

std::vector<std::tuple<std::string, int, std::string, std::string>> Database::session_list() const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    return storage.select(asterisk<SessionInfo>());
}

std::optional<SessionInfo> Database::session(const std::string& session_id) const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    return storage.get_optional<SessionInfo>(session_id);
}

void Database::add_session(const std::string& session_id, int user_id, const std::string& creation_date, const std::string& max_age_time) const
{
    const SessionInfo session{
        .id = session_id,
        .user_id = user_id,
        .creation_date = creation_date,
        .max_age_time = max_age_time
    };

    const std::unique_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    storage.replace(session);
}

bool Database::delete_session(const std::string& session_id) const
{
    const std::unique_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    const std::optional success{
        storage.get_optional<SessionInfo>(session_id)
            .transform([&](const SessionInfo& session) -> bool {
                storage.remove<SessionInfo>(session.id);
                return true;
            })
            .or_else([&] -> std::optional<bool> {
                logging::error{ R"(Fail to delete session "{}")", session_id };
                return false;
            })
    };
    return success.value_or(false);
}

std::filesystem::path Database::base_path() const
{
    return _path.parent_path();
}

std::filesystem::path Database::video_path() const
{
    return base_path() / "videos";
}

std::filesystem::path Database::video_path(int video_id) const
{
    return video_path() / su::int_to_string(video_id);
}

std::filesystem::path Database::thumbnail_path() const
{
    return base_path() / "thumbnails";
}

std::filesystem::path Database::thumbnail_path(int video_id) const
{
    return thumbnail_path() / su::int_to_string(video_id);
}

std::string Database::hls_video_name(int video_id)
{
    return su::int_to_string(video_id);
}

std::filesystem::path Database::hls_video_path(int video_id) const
{
    return video_path() / ("hls_" + su::int_to_string(video_id));
}
