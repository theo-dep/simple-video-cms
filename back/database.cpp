#include "database.h"

#include "logging.h"
#include "servercommon.h"
#include "stringutils.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wchanges-meaning"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"
extern "C" {
#include <unqlite.h>
}
#include <unqlitepp.hpp>
#pragma GCC diagnostic pop

#include <memory>
#include <ranges>

namespace database
{
    std::optional<std::pair<up::db, up::vm>> execute(const std::filesystem::path& path, up::db_mode mode, const std::string_view& jx9_program) noexcept;
    std::optional<std::pair<up::db, up::vm>> execute(const std::filesystem::path& path, up::db_mode mode, const std::string_view& jx9_program,
                                                     const std::unordered_map<std::string, up::value>& binding_map) noexcept;
}

Database::Database(const std::filesystem::path& path, bool& create_ok) noexcept
    : path_{ path }
{
    create_ok = true;
    if (!std::filesystem::exists(path_)) {
        up::db_make_status status;
        const std::optional db{ up::db::make(path, up::db_mode::OPEN_CREATE, &status) };
        if (!db.has_value()) {
            logging::error{ "{}: {}", up::status_to_string_view<up::db_make_status>::value, static_cast<std::size_t>(status) };
            create_ok = false;
        }
    }
}

bool Database::create_tables() const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
            if(!db_exists('users')) {
                $rc = db_create('users');
                if (!$rc) {
                    print db_errlog();
                    return;
                }

                $schema =  {
                    username: 'string',
                    password: 'string',
                    salt: 'string'
                };
                db_set_schema('users', $schema);
            }
            if(!db_exists('admins')) {
                $rc = db_create('admins');
                if (!$rc) {
                    print db_errlog();
                    return;
                }

                $schema =  {
                    username: 'string',
                    password: 'string',
                    salt: 'string',
                    super: 'bool'
                };
                db_set_schema('admins', $schema);
            }
            if(!db_exists('videos')) {
                $rc = db_create('videos');
                if (!$rc) {
                    print db_errlog();
                    return;
                }

                $schema =  {
                    title: 'string',
                    video: 'resource',
                    view_count: 'integer'
                };
                db_set_schema('videos', $schema);
            }
            if(!db_exists('video_rights')) {
                $rc = db_create('video_rights');
                if (!$rc) {
                    print db_errlog();
                    return;
                }

                $schema =  {
                    video_id: 'integer',
                    user_id: 'integer'
                };
                db_set_schema('video_rights', $schema);
            }
        )"sv };

    const std::optional db_vm{ database::execute(path_, up::db_mode::OPEN_READWRITE, jx9_prog) };
    return db_vm.has_value();
}

std::vector<std::int64_t> Database::video_list() const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $records = db_fetch_all('videos');
    )"sv };

    std::optional db_vm{ database::execute(path_, up::db_mode::OPEN_READONLY, jx9_prog) };
    if (!db_vm.has_value()) {
        logging::error{ "Fail to fetch all videos" };
        return {};
    }

    std::vector<std::int64_t> videos;
    const std::function video_callback{
        [&videos](const std::string& key, const up::vm_value& value) -> bool {
            if (key == "__id") {
                videos.push_back(value.get_int());
                return true;
            }
            return false;
        }
    };

    const std::optional records{ db_vm->second.extract("records") };
    if (!records->foreach_object(video_callback)) {
        logging::error{ "Fail to fetch all videos" };
        return {};
    }

    return videos;
}

std::vector<std::int64_t> Database::video_list(const std::string& username) const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $callback = function($record) {
            $record_id = $record.__id;
            $video_right_record = db_fetch_by_id('video_rights', $record_id);
            if ($video_right_record == NULL) {
                return TRUE; // no user associated == for all users
            }
            return ($video_right_record.user_id == $username);
        };

        $records = db_fetch_all('videos', $callback);
    )"sv };

    std::optional db_vm{ database::execute(path_, up::db_mode::OPEN_READONLY, jx9_prog, { { "username", username } }) };
    if (!db_vm.has_value()) {
        logging::error{ "Fail to fetch all videos" };
        return {};
    }

    std::vector<std::int64_t> videos;
    const std::function video_callback{
        [&videos](const std::string& key, const up::vm_value& value) -> bool {
            if (key == "__id") {
                videos.push_back(value.get_int());
                return true;
            }
            return false;
        }
    };

    const std::optional records{ db_vm->second.extract("records") };
    if (!records->foreach_object(video_callback)) {
        logging::error{ "Fail to fetch all videos" };
        return {};
    }

    return videos;
}

std::vector<std::int64_t> Database::no_right_video_list() const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $callback = function($record) {
            $record_id = $record.__id;
            $video_right_record = db_fetch_by_id('video_rights', $record_id);
            return ($video_right_record == NULL); // no user associated == for all users
        };

        $records = db_fetch_all('videos', $callback);
    )"sv };

    std::optional db_vm{ database::execute(path_, up::db_mode::OPEN_READONLY, jx9_prog) };
    if (!db_vm.has_value()) {
        logging::error{ "Fail to fetch all videos" };
        return {};
    }

    std::vector<std::int64_t> videos;
    const std::function video_callback{
        [&videos](const std::string& key, const up::vm_value& value) -> bool {
            if (key == "__id") {
                videos.push_back(value.get_int());
                return true;
            }
            return false;
        }
    };

    const std::optional records{ db_vm->second.extract("records") };
    if (!records->foreach_object(video_callback)) {
        logging::error{ "Fail to fetch all videos" };
        return {};
    }

    return videos;
}

std::string Database::video_title(const std::int64_t& id) const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $record = db_fetch_by_id('videos', $id);
    )"sv };

    std::optional db_vm{ database::execute(path_, up::db_mode::OPEN_READONLY, jx9_prog, { { "id", id } }) };
    if (!db_vm.has_value()) {
        logging::error{ "Fail to fetch video title: {}", id };
        return {};
    }

    const std::optional record{ db_vm->second.extract("record") };
    if (!record.has_value()) {
        logging::error{ "Fail to fetch video title: {}", id };
        return {};
    }

    return record->get_string();
}

#if 0
int database::video_views(const std::int64_t& id) const noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        logging::error{ "Fail to open database connection" };
        return -1;
    }

    return video(conn, id).value_or(Video{}).view_count;
}

std::string database::video_file_path(const std::int64_t& id) const noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        logging::error{ "Fail to open database connection" };
        return {};
    }

    return video(conn, id).value_or(Video{}).file_path;
}

std::optional<Admin> database::admin(const std::unique_ptr<dbng>& conn, const std::string& username) const noexcept
{
    const std::vector admins{ conn->query_s<Admin>("username=?", username) };
    return (admins.empty() ? std::nullopt : std::optional{ admins[0] });
}

std::optional<User> database::user(const std::unique_ptr<dbng>& conn, const std::string& username) const noexcept
{
    const std::vector users{ conn->query_s<User>("username=?", username) };
    return (users.empty() ? std::nullopt : std::optional{ users[0] });
}

void database::add_super_admin(const std::string& username, const std::string& password, const std::string& salt) const noexcept
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

bool database::is_super_admin(const std::string& username) const noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        logging::error{ "Fail to open database connection" };
        return false;
    }

    return admin(conn, username).value_or(Admin{}).super;
}

bool database::is_admin(const std::string& username) const noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        logging::error{ "Fail to open database connection" };
        return false;
    }

    return admin(conn, username).has_value();
}

bool database::is_user(const std::string& username) const noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        logging::error{ "Fail to open database connection" };
        return false;
    }

    return user(conn, username).has_value();
}

void database::add_admin(const std::string& username, const std::string& password, const std::string& salt) const noexcept
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

void database::add_user(const std::string& username, const std::string& password, const std::string& salt) const noexcept
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

void database::update_admin(const std::string& username, const std::string& password) const noexcept
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

void database::update_user(const std::string& username, const std::string& password) const noexcept
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

void database::delete_admin(const std::string& username) const noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        logging::error{ "Fail to open database connection" };
        return;
    }

    conn->delete_records_s<Admin>("username=?", username);
}

void database::delete_user(const std::string& username) const noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        logging::error{ "Fail to open database connection" };
        return;
    }

    conn->delete_records_s<User>("username=?", username);
}

std::string database::user_password(const std::string& username) const noexcept
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

std::string database::user_salt(const std::string& username) const noexcept
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

int database::user_count() const noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        logging::error{ "Fail to open database connection" };
        return -1;
    }

    return static_cast<int>(conn->query_s<User>().size());
}

int database::video_count() const noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        logging::error{ "Fail to open database connection" };
        return -1;
    }

    return static_cast<int>(conn->query_s<Video>().size());
}

int database::view_count() const noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        logging::error{ "Fail to open database connection" };
        return -1;
    }

    const std::vector view_counts{ conn->query_s<std::tuple<int>>("SELECT SUM(view_count) FROM Video") };
    return (view_counts.empty() ? -1 : std::get<0>(view_counts[0]));
}

std::vector<std::string> database::user_list() const noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        logging::error{ "Fail to open database connection" };
        return {};
    }

    const std::vector tuple_usernames{ conn->query_s<std::tuple<std::string>>("SELECT username FROM User") };
    return transform(tuple_usernames);
}

std::vector<std::string> database::admin_list() const noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        logging::error{ "Fail to open database connection" };
        return {};
    }

    const std::vector tuple_usernames{ conn->query_s<std::tuple<std::string>>("SELECT username FROM Admin") };
    return transform(tuple_usernames);
}

void database::add_video(const std::int64_t& id, const std::string& title, const std::string& file_path) const noexcept
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

void database::add_video_rights(const std::int64_t& id, const std::vector<std::string>& usernames) const noexcept
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

void database::update_video_rights(const std::int64_t& id, const std::vector<std::string>& usernames) const noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        logging::error{ "Fail to open database connection" };
        return;
    }

    conn->delete_records_s<VideoRight>("video_id=?", id);

    add_video_rights(id, usernames);
}

void database::delete_video(const std::int64_t& id) const noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        logging::error{ "Fail to open database connection" };
        return;
    }

    conn->delete_records_s<Video>("id=?", id);
}

void database::increment_video_views(const std::int64_t& id) const noexcept
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

bool database::has_video_right(const std::int64_t& id) const noexcept
{
    const std::unique_ptr conn{ connection() };
    if (conn == nullptr) {
        logging::error{ "Fail to open database connection" };
        return false;
    }

    const std::vector videos_rights{ conn->query_s<VideoRight>("video_id=?", id) };
    return videos_rights.empty();
}

bool database::has_video_right(const std::int64_t& id, const std::string& username) const noexcept
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

std::vector<std::string> database::video_right_list(const std::int64_t& id) const noexcept
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
#endif

std::optional<std::pair<up::db, up::vm>> database::execute(const std::filesystem::path& path, up::db_mode mode, const std::string_view& jx9_program) noexcept
{
    return execute(path, mode, jx9_program, {});
}

std::optional<std::pair<up::db, up::vm>> database::execute(const std::filesystem::path& path, up::db_mode mode, const std::string_view& jx9_program,
                                                           const std::unordered_map<std::string, up::value>& binding_map) noexcept
{
    up::db_make_status make_status;
    std::optional db{ up::db::make(path, mode, &make_status) };
    if (!db.has_value()) {
        logging::error{ "{}: {}", up::status_to_string_view<up::db_make_status>::value, static_cast<std::size_t>(make_status) };
        return std::nullopt;
    }

    up::db_compilation_status compile_status;
    std::string_view compile_error;
    std::optional vm{ db->compile(jx9_program, &compile_status, &compile_error) };
    if (!vm.has_value()) {
        logging::error{ "{}: {}. {}",
                        up::status_to_string_view<up::db_compilation_status>::value, static_cast<std::size_t>(compile_status), compile_error };
        return std::nullopt;
    }

    for (const auto& [key, value] : binding_map) {
        if (!vm->bind(key, value)) {
            logging::error{ "Fail to bind key: {}", key };
            return std::nullopt;
        }
    }

    up::vm_execute_status exe_status;
    if (!vm->exec(&exe_status)) {
        logging::error{ "{}: {}", up::status_to_string_view<up::vm_execute_status>::value, static_cast<std::size_t>(exe_status) };
        return std::nullopt;
    }

    return std::make_pair<up::db, up::vm>(std::move(db.value()), std::move(vm.value()));
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wchanges-meaning"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#pragma GCC diagnostic ignored "-Wregister"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-but-set-parameter"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wwrite-strings"
extern "C" {
#include <unqlite.c>
}
#pragma GCC diagnostic pop
