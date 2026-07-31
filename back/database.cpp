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
    };
    static constexpr Version current_version{ .value = 1 };
}

inline auto database::storage(const std::filesystem::path& path)
{
    return make_storage(
        path.string(),
        make_table("version",
                   make_column("value", &Version::value)),
        make_table("users",
                   make_column("id", &User::id, primary_key().autoincrement()),
                   make_column("name", &User::name, unique(), collate_nocase()),
                   make_column("password", &User::password),
                   make_column("salt", &User::salt)),
        make_table("super_admins",
                   make_column("id", &SuperAdmin::id, primary_key()),
                   foreign_key(&SuperAdmin::id).references(&User::id).on_delete.cascade()),
        make_table("admins",
                   make_column("id", &Admin::id, primary_key()),
                   foreign_key(&Admin::id).references(&User::id).on_delete.cascade()),
        make_table("groups",
                   make_column("id", &Group::id, primary_key().autoincrement()),
                   make_column("name", &Group::name, unique(), collate_nocase())),
        make_table("videos",
                   make_column("id", &Video::id, primary_key().autoincrement()),
                   make_column("title", &Video::title, unique(), collate_nocase())),
        make_table("user_groups",
                   make_column("group_id", &GroupUser::group_id),
                   make_column("user_id", &GroupUser::user_id),
                   primary_key(&GroupUser::group_id, &GroupUser::user_id),
                   foreign_key(&GroupUser::group_id).references(&Group::id).on_delete.cascade(),
                   foreign_key(&GroupUser::user_id).references(&User::id).on_delete.cascade()),
        make_table("video_user_rights",
                   make_column("video_id", &VideoUserRight::video_id),
                   make_column("user_id", &VideoUserRight::user_id),
                   primary_key(&VideoUserRight::video_id, &VideoUserRight::user_id),
                   foreign_key(&VideoUserRight::video_id).references(&Video::id).on_delete.cascade(),
                   foreign_key(&VideoUserRight::user_id).references(&User::id).on_delete.cascade()),
        make_table("video_group_rights",
                   make_column("video_id", &VideoGroupRight::video_id),
                   make_column("group_id", &VideoGroupRight::group_id),
                   primary_key(&VideoGroupRight::video_id, &VideoGroupRight::group_id),
                   foreign_key(&VideoGroupRight::video_id).references(&Video::id).on_delete.cascade(),
                   foreign_key(&VideoGroupRight::group_id).references(&Group::id).on_delete.cascade()),
        make_table("user_video_bookmarks",
                   make_column("user_id", &UserVideoBookmark::user_id),
                   make_column("video_id", &UserVideoBookmark::video_id),
                   primary_key(&UserVideoBookmark::user_id, &UserVideoBookmark::video_id),
                   foreign_key(&UserVideoBookmark::user_id).references(&User::id).on_delete.cascade(),
                   foreign_key(&UserVideoBookmark::video_id).references(&Video::id).on_delete.cascade()));
    // make virtual table with FTS5 for search?
    // https://www.sqlite.org/fts5.html
}

namespace database
{
    using StorageType = decltype(database::storage({}));

    constexpr auto video_struct = struct_<Video>(&Video::id, &Video::title);
    constexpr auto group_struct = struct_<Group>(&Group::id, &Group::name);
    constexpr auto user_struct = struct_<User>(&User::id, &User::name);
}

Database::Database(std::filesystem::path path)
    : _path{ std::move(path) }
{
}

bool Database::create_tables() const
{
    const std::unique_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    storage.sync_schema();

    const std::vector versions{ storage.get_all<database::Version>() };
    if (!versions.empty() && versions.at(0).value == database::current_version.value)
        return true;

    storage.transaction([&] {
        // migrate
        const std::vector users{ storage.get_all<User>() };
        const std::vector super_admins{ storage.get_all<SuperAdmin>() };
        const std::vector admins{ storage.get_all<Admin>() };
        const std::vector groups{ storage.get_all<Group>() };
        const std::vector videos{ storage.get_all<Video>() };
        const std::vector user_groups{ storage.get_all<GroupUser>() };
        const std::vector video_user_rights{ storage.get_all<VideoUserRight>() };
        const std::vector video_group_rights{ storage.get_all<VideoGroupRight>() };
        const std::vector user_video_bookmarks{ storage.get_all<UserVideoBookmark>() };

        for (const std::string& table : {
                 "user_video_bookmarks",
                 "video_group_rights", "video_user_rights", "user_groups",
                 "videos", "groups", "admins", "super_admins", "users" }) {
            storage.drop_table(table);
        }

        storage.sync_schema();

        const auto insert_all{ [&storage](const auto& vec) {
            for (const auto& item : vec) {
                storage.replace(item);
            }
        } };

        const std::tuple vecs{ std::tie(users, super_admins, admins, groups, videos, user_groups, video_user_rights, video_group_rights, user_video_bookmarks) };
        std::apply([&](const auto&... vecs) { (insert_all(vecs), ...); }, vecs);

        storage.replace(database::current_version);

        return true;
    });

    return true;
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

bool Database::video_exists(const std::string& title) const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    const std::vector videos{ storage.select(&Video::id, where(c(&Video::title) == title)) };
    return videos.size() == 1;
}

bool Database::video_exists(int video_id, const std::string& title) const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    const std::vector videos{ storage.select(&Video::id, where(c(&Video::title) == title and c(&Video::id) != video_id)) };
    return videos.size() == 1;
}

std::string Database::video_title(int video_id) const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    const std::optional video_title{
        storage.get_optional<Video>(video_id)
            .transform([](const Video& video) -> std::string {
                return video.title;
            })
            .or_else([&] -> std::optional<std::string> {
                logging::error{ R"(Fail to fetch video title "{}")", video_id };
                return std::string{};
            })
    };
    return video_title.value_or(std::string{});
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

int Database::user_id(const std::string& name) const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    const std::vector users{ storage.select(&User::id, where(c(&User::name) == name)) };
    return users.size() == 1 ? users.at(0) : -1;
}

bool Database::user_exists(const std::string& name) const
{
    return user_id(name) != -1;
}

bool Database::user_exists(int user_id, const std::string& name) const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    const std::vector users{ storage.select(&User::id, where(c(&User::name) == name and c(&User::id) != user_id)) };
    return users.size() == 1;
}

std::string Database::user_name(int user_id) const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    const std::optional user_name{
        storage.get_optional<User>(user_id)
            .transform([](const User& user) -> std::string {
                return user.name;
            })
            .or_else([&] -> std::optional<std::string> {
                logging::error{ R"(Fail to fetch user name "{}")", user_id };
                return std::string{};
            })
    };
    return user_name.value_or(std::string{});
}

std::optional<std::string> Database::user_password(int user_id) const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    const std::optional user_password{
        storage.get_optional<User>(user_id)
            .and_then([](const User& user) -> std::optional<std::string> {
                return user.password;
            })
        // can be null
        //.or_else([&] -> std::optional<std::string> {
        //    logging::error{ R"(Fail to fetch user password "{}")", user_id };
        //    return std::nullopt;
        //})
    };
    return user_password;
}

std::string Database::user_salt(int user_id) const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    const std::optional user_salt{
        storage.get_optional<User>(user_id)
            .transform([](const User& user) -> std::string {
                return user.salt;
            })
            .or_else([&] -> std::optional<std::string> {
                logging::error{ R"(Fail to fetch user salt "{}")", user_id };
                return std::string{};
            })
    };
    return user_salt.value_or(std::string{});
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

std::string Database::group_name(int group_id) const
{
    const std::shared_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    const std::optional group_name{
        storage.get_optional<Group>(group_id)
            .transform([](const Group& group) -> std::string {
                return group.name;
            })
            .or_else([&] -> std::optional<std::string> {
                logging::error{ R"(Fail to fetch group name "{}")", group_id };
                return std::string{};
            })
    };
    return group_name.value_or(std::string{});
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

std::optional<int> Database::add_video(const std::string& title, const std::string& video_content) const
{
    if (!filesystem::create(video_path())) {
        return std::nullopt;
    }

    Video video{
        .title = title
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

std::optional<int> Database::update_video_title(int video_id, const std::string& title) const
{
    const std::unique_lock lock(_mutex);
    database::StorageType storage{ database::storage(_path) };
    const std::optional id{
        storage.get_optional<Video>(video_id)
            .and_then([&](Video video) -> std::optional<int> {
                video.title = title;
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
