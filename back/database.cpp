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

    up::vm pair_to_vm(std::pair<up::db, up::vm> pair) noexcept;
    std::optional<up::vm_value> extract_variable(up::vm vm, const std::string& record_name) noexcept;
    std::optional<std::vector<up::vm_value>> extract_variables(up::vm vm, const std::vector<std::string>& record_names) noexcept;
    std::optional<up::vm_value> find_member(const up::vm_value& value, const std::string& member_name) noexcept;
    std::optional<std::vector<up::vm_value>> find_members(const up::vm_value& value, const std::vector<std::string>& member_names) noexcept;
    std::vector<std::int64_t> value_to_ids(const up::vm_value& value) noexcept;
    std::vector<up::value> ids_to_values(const std::vector<std::int64_t>& ids) noexcept;
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
            if (!db_exists('users')) {
                $rc = db_create('users');
                if (!$rc) {
                    print db_errlog();
                    return;
                }

                $schema =  {
                    name: 'string',
                    password: 'string',
                    salt: 'string'
                };
                db_set_schema('users', $schema);
            }
            if (!db_exists('admins')) {
                $rc = db_create('admins');
                if (!$rc) {
                    print db_errlog();
                    return;
                }

                $schema =  {
                    name: 'string',
                    password: 'string',
                    salt: 'string',
                    super: 'bool'
                };
                db_set_schema('admins', $schema);
            }
            if (!db_exists('videos')) {
                $rc = db_create('videos');
                if (!$rc) {
                    print db_errlog();
                    return;
                }

                $schema =  {
                    title: 'string',
                    video: 'resource',
                    video_size: 'integer',
                    views: 'integer'
                };
                db_set_schema('videos', $schema);
            }
            if (!db_exists('video_rights')) {
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
        $videos = db_fetch_all('videos');
    )"sv };

    const std::optional videos{
        database::execute(path_, up::db_mode::OPEN_READONLY, jx9_prog)
            .transform(database::pair_to_vm)
            .and_then(std::bind(database::extract_variable, std::placeholders::_1, "videos"s))
            .transform(database::value_to_ids)
    };

    if (!videos.has_value()) {
        logging::error{ "Fail to fetch all videos" };
        return {};
    }

    return videos.value();
}

std::vector<std::int64_t> Database::video_list(const std::int64_t& user_id) const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $callback = function($video) {
            $video_id = $video.__id;

            $video_right_callback = function($right) {
                return ($right.video_id == $video_id && $right.user_id == $user_id);
            };
            $video_rights = db_fetch_all('video_rights', $video_right_callback);

            return (count($video_rights) != 0);
        };

        $videos = db_fetch_all('videos', $callback);
    )"sv };

    const std::optional videos{
        database::execute(path_, up::db_mode::OPEN_READONLY, jx9_prog, { { "user_id"s, user_id } })
            .transform(database::pair_to_vm)
            .and_then(std::bind(database::extract_variable, std::placeholders::_1, "videos"s))
            .transform(database::value_to_ids)
    };

    if (!videos.has_value()) {
        logging::error{ "Fail to fetch all videos for user \"{}\"", user_id };
        return {};
    }

    return videos.value();
}

std::vector<std::int64_t> Database::no_right_video_list() const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $callback = function($videos) {
            $videos_id = $videos.__id;

            $video_right_callback = function($right) {
                return ($right.video_id == $video_id);
            };
            $video_rights = db_fetch_all('video_rights', $video_right_callback);
            return (count($video_rights) == 0);
        };

        $videos = db_fetch_all('videos', $callback);
    )"sv };

    const std::optional videos{
        database::execute(path_, up::db_mode::OPEN_READONLY, jx9_prog)
            .transform(database::pair_to_vm)
            .and_then(std::bind(database::extract_variable, std::placeholders::_1, "videos"s))
            .transform(database::value_to_ids)
    };

    if (!videos.has_value()) {
        logging::error{ "Fail to fetch all no right videos" };
        return {};
    }

    return videos.value();
}

std::string Database::video_title(const std::int64_t& id) const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $video = db_fetch_by_id('videos', $id);
    )"sv };

    const std::optional title{
        database::execute(path_, up::db_mode::OPEN_READONLY, jx9_prog, { { "id"s, id } })
            .transform(database::pair_to_vm)
            .and_then(std::bind(database::extract_variable, std::placeholders::_1, "video"s))
            .and_then(std::bind(database::find_member, std::placeholders::_1, "title"s))
    };

    if (!title.has_value()) {
        logging::error{ "Fail to fetch video title \"{}\"", id };
        return {};
    }

    return title->get_string();
}

std::int64_t Database::video_views(const std::int64_t& id) const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $video = db_fetch_by_id('videos', $id);
    )"sv };

    const std::optional views{
        database::execute(path_, up::db_mode::OPEN_READONLY, jx9_prog, { { "id"s, id } })
            .transform(database::pair_to_vm)
            .and_then(std::bind(database::extract_variable, std::placeholders::_1, "video"s))
            .and_then(std::bind(database::find_member, std::placeholders::_1, "views"s))
    };

    if (!views.has_value()) {
        logging::error{ "Fail to fetch video views \"{}\"", id };
        return {};
    }

    return views->get_int();
}

std::string Database::video(const std::int64_t& id) const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $video = db_fetch_by_id('videos', $id);
    )"sv };

    const std::optional video{
        database::execute(path_, up::db_mode::OPEN_READONLY, jx9_prog, { { "id"s, id } })
            .transform(database::pair_to_vm)
            .and_then(std::bind(database::extract_variable, std::placeholders::_1, "video"s))
            .and_then(std::bind(database::find_members, std::placeholders::_1, std::vector{ "video"s, "video_size"s }))
    };

    if (!video.has_value() || video->size() != 2) {
        logging::error{ "Fail to fetch video \"{}\"", id };
        return {};
    }

    return std::string(reinterpret_cast<const char*>((*video)[0].get_resource()), (*video)[1].get_int());
}

std::optional<std::int64_t> Database::add_super_admin(const std::string& name, const std::string& password, const std::string& salt) const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $success = db_store('admins', $admin);
        $admin_id = $admin.__id;
    )"sv };

    const std::optional success{
        database::execute(path_, up::db_mode::OPEN_READWRITE, jx9_prog,
                          { { "admin"s, up::value::object{
                                            { "name"s, name }, { "password"s, password }, { "salts"s, salt }, { "super"s, true } } } })
            .transform(database::pair_to_vm)
            .and_then(std::bind(database::extract_variables, std::placeholders::_1, std::vector{ "success"s, "admin_id"s }))
    };

    if (!success.has_value() || success->size() != 2 || !(*success)[0].get_bool()) {
        logging::error{ "Fail to add super admin \"{}\"", name };
        return {};
    }

    return (*success)[1].get_int();
}

bool Database::is_super_admin(const std::int64_t& id) const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $admin = db_fetch_by_id('admins', $id);
    )"sv };

    const std::optional super{
        database::execute(path_, up::db_mode::OPEN_READONLY, jx9_prog, { { "id"s, id } })
            .transform(database::pair_to_vm)
            .and_then(std::bind(database::extract_variable, std::placeholders::_1, "admin"s))
            .and_then(std::bind(database::find_member, std::placeholders::_1, "super"s))
    };

    if (!super.has_value()) {
        logging::error{ "Fail to get super admin \"{}\"", id };
        return false;
    }

    return super->get_bool();
}

bool Database::is_admin(const std::int64_t& id) const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $admin = db_fetch_by_id('admins', $id);
    )"sv };

    const std::optional record{
        database::execute(path_, up::db_mode::OPEN_READONLY, jx9_prog, { { "id"s, id } })
            .transform(database::pair_to_vm)
            .and_then(std::bind(database::extract_variable, std::placeholders::_1, "admin"s))
    };

    if (!record.has_value()) {
        logging::error{ "Fail to get admin \"{}\"", id };
        return false;
    }

    return !record->is_null();
}

bool Database::is_user(const std::int64_t& id) const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $user = db_fetch_by_id('users', $id);
    )"sv };

    const std::optional record{
        database::execute(path_, up::db_mode::OPEN_READONLY, jx9_prog, { { "id"s, id } })
            .transform(database::pair_to_vm)
            .and_then(std::bind(database::extract_variable, std::placeholders::_1, "user"s))
    };

    if (!record.has_value()) {
        logging::error{ "Fail to get user \"{}\"", id };
        return false;
    }

    return !record->is_null();
}

std::optional<std::int64_t> Database::add_admin(const std::string& name, const std::string& password, const std::string& salt) const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $success = db_store('admins', $admin);
        $admin_id = $admin.__id;
    )"sv };

    const std::optional success{
        database::execute(path_, up::db_mode::OPEN_READWRITE, jx9_prog,
                          { { "admin"s, up::value::object{
                                            { "name"s, name }, { "password"s, password }, { "salt"s, salt }, { "super"s, false } } } })
            .transform(database::pair_to_vm)
            .and_then(std::bind(database::extract_variables, std::placeholders::_1, std::vector{ "success"s, "admin_id"s }))
    };

    if (!success.has_value() || success->size() != 2 || !(*success)[0].get_bool()) {
        logging::error{ "Fail to add admin \"{}\"", name };
        return false;
    }

    return (*success)[1].get_int();
}

std::optional<std::int64_t> Database::add_user(const std::string& name, const std::string& password, const std::string& salt) const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $success = db_store('users', $user);
        $user_id = $user.__id;
    )"sv };

    const std::optional success{
        database::execute(path_, up::db_mode::OPEN_READWRITE, jx9_prog,
                          { { "user"s, up::value::object{
                                           { "name"s, name }, { "password"s, password }, { "salt"s, salt } } } })
            .transform(database::pair_to_vm)
            .and_then(std::bind(database::extract_variables, std::placeholders::_1, std::vector{ "success"s, "user_id"s }))
    };

    if (!success.has_value() || success->size() != 2 || !(*success)[0].get_bool()) {
        logging::error{ "Fail to add user \"{}\"", name };
        return false;
    }

    return (*success)[1].get_int();
}

bool Database::update_admin(const std::int64_t& id, const std::string& password) const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $admin = db_fetch_by_id('admins', $id);
        $drop_success = db_drop_record('admins', $id);
        if ($drop_success) {
            $admin.password = $password;
            $store_success = db_store('admin', $admin);
        }
    )"sv };

    const std::optional success{
        database::execute(path_, up::db_mode::OPEN_READWRITE, jx9_prog, { { "id"s, id }, { "password"s, password } })
            .transform(database::pair_to_vm)
            .and_then(std::bind(database::extract_variables, std::placeholders::_1, std::vector{ "drop_success"s, "store_success"s }))
    };

    if (!success.has_value() || success->size() != 2) {
        logging::error{ "Fail to update admin \"{}\"", id };
        return false;
    }

    return ((*success)[0].get_bool() && (*success)[1].get_bool());
}

bool Database::update_user(const std::int64_t& id, const std::string& password) const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $user = db_fetch_by_id('users', $id);
        $drop_success = db_drop_record('users', $id);
        if ($drop_success) {
            $user.password = $password;
            $store_success = db_store('users', $user);
        }
    )"sv };

    const std::optional success{
        database::execute(path_, up::db_mode::OPEN_READWRITE, jx9_prog, { { "id"s, id }, { "password"s, password } })
            .transform(database::pair_to_vm)
            .and_then(std::bind(database::extract_variables, std::placeholders::_1, std::vector{ "drop_success"s, "store_success"s }))
    };

    if (!success.has_value() || success->size() != 2) {
        logging::error{ "Fail to update user \"{}\"", id };
        return false;
    }

    return ((*success)[0].get_bool() && (*success)[1].get_bool());
}

bool Database::delete_admin(const std::int64_t& id) const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $success = db_drop_record('admins', $id);
    )"sv };

    const std::optional success{
        database::execute(path_, up::db_mode::OPEN_READWRITE, jx9_prog, { { "id"s, id } })
            .transform(database::pair_to_vm)
            .and_then(std::bind(database::extract_variable, std::placeholders::_1, "success"s))
    };

    if (!success.has_value()) {
        logging::error{ "Fail to delete admin \"{}\"", id };
        return false;
    }

    return success->get_bool();
}

bool Database::delete_user(const std::int64_t& id) const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $success = db_drop_record('users', $id);
    )"sv };

    const std::optional success{
        database::execute(path_, up::db_mode::OPEN_READWRITE, jx9_prog, { { "id"s, id } })
            .transform(database::pair_to_vm)
            .and_then(std::bind(database::extract_variable, std::placeholders::_1, "success"s))
    };

    if (!success.has_value()) {
        logging::error{ "Fail to delete user \"{}\"", id };
        return false;
    }

    return success->get_bool();
}

std::int64_t Database::user_id(const std::string& name) const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $callback = function($user) {
            return ($user.name == $username);
        };
        $users = db_fetch_all($collection, $callback);
    )"sv };

    const auto fetch_id_function{
        [&](const std::string& collection) -> std::optional<std::vector<std::int64_t>> {
            return database::execute(path_, up::db_mode::OPEN_READONLY, jx9_prog, { { "username"s, name }, { "collection"s, collection } })
                .transform(database::pair_to_vm)
                .and_then(std::bind(database::extract_variable, std::placeholders::_1, "users"s))
                .transform(database::value_to_ids);
        }
    };

    std::optional ids{ fetch_id_function("users") };
    if (!ids.has_value() || ids->empty()) {
        ids = fetch_id_function("admins");
    }

    if (!ids.has_value() || ids->empty()) {
        logging::error{ "Fail to fetch user \"{}\"", name };
        return 0;
    }

    return (*ids)[0];
}

std::string Database::user_name(const std::int64_t& id) const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $user = db_fetch_by_id($collection, $id);
    )"sv };

    const auto fetch_name_function{
        [&](const std::string& collection) -> std::optional<up::vm_value> {
            return database::execute(path_, up::db_mode::OPEN_READONLY, jx9_prog, { { "id"s, id }, { "collection"s, collection } })
                .transform(database::pair_to_vm)
                .and_then(std::bind(database::extract_variable, std::placeholders::_1, "user"s))
                .and_then(std::bind(database::find_member, std::placeholders::_1, "name"s));
        }
    };

    std::optional name{ fetch_name_function("users") };
    if (!name.has_value()) {
        name = fetch_name_function("admins");
    }

    if (!name.has_value()) {
        logging::error{ "Fail to fetch user name \"{}\"", id };
        return {};
    }

    return name->get_string();
}

std::string Database::user_password(const std::int64_t& id) const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $user = db_fetch_by_id($collection, $id);
    )"sv };

    const auto fetch_password_function{
        [&](const std::string& collection) -> std::optional<up::vm_value> {
            return database::execute(path_, up::db_mode::OPEN_READONLY, jx9_prog, { { "id"s, id }, { "collection"s, collection } })
                .transform(database::pair_to_vm)
                .and_then(std::bind(database::extract_variable, std::placeholders::_1, "user"s))
                .and_then(std::bind(database::find_member, std::placeholders::_1, "password"s));
        }
    };

    std::optional password{ fetch_password_function("users") };
    if (!password.has_value()) {
        password = fetch_password_function("admins");
    }

    if (!password.has_value()) {
        logging::error{ "Fail to fetch user password \"{}\"", id };
        return {};
    }

    return password->get_string();
}

std::string Database::user_salt(const std::int64_t& id) const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $user = db_fetch_by_id($collection, $id);
    )"sv };

    const auto fetch_salt_function{
        [&](const std::string& collection) -> std::optional<up::vm_value> {
            return database::execute(path_, up::db_mode::OPEN_READONLY, jx9_prog, { { "id"s, id }, { "collection"s, collection } })
                .transform(database::pair_to_vm)
                .and_then(std::bind(database::extract_variable, std::placeholders::_1, "user"s))
                .and_then(std::bind(database::find_member, std::placeholders::_1, "salt"s));
        }
    };

    std::optional salt{ fetch_salt_function("users") };
    if (!salt.has_value()) {
        salt = fetch_salt_function("admins");
    }

    if (!salt.has_value()) {
        logging::error{ "Fail to fetch user salt \"{}\"", id };
        return {};
    }

    return salt->get_string();
}

std::uint32_t Database::user_count() const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $count = db_total_records('users');
    )"sv };

    const std::optional count{
        database::execute(path_, up::db_mode::OPEN_READONLY, jx9_prog)
            .transform(database::pair_to_vm)
            .and_then(std::bind(database::extract_variable, std::placeholders::_1, "count"s))
    };

    if (!count.has_value()) {
        logging::error{ "Fail to get user count" };
        return 0;
    }

    return static_cast<std::uint32_t>(count->get_int());
}

std::uint32_t Database::video_count() const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $count = db_total_records('videos');
    )"sv };

    const std::optional count{
        database::execute(path_, up::db_mode::OPEN_READONLY, jx9_prog)
            .transform(database::pair_to_vm)
            .and_then(std::bind(database::extract_variable, std::placeholders::_1, "count"s))
    };

    if (!count.has_value()) {
        logging::error{ "Fail to get video count" };
        return 0;
    }

    return static_cast<std::uint32_t>(count->get_int());
}

std::uint32_t Database::view_count() const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $videos = db_fetch_all('videos');
        $views_callback = function($video) {
            return $video.views;
        };
        $count = array_sum(array_walk($views_callback, $videos));
    )"sv };

    const std::optional count{
        database::execute(path_, up::db_mode::OPEN_READONLY, jx9_prog)
            .transform(database::pair_to_vm)
            .and_then(std::bind(database::extract_variable, std::placeholders::_1, "count"s))
    };

    if (!count.has_value()) {
        logging::error{ "Fail to get view count" };
        return 0;
    }

    return static_cast<std::uint32_t>(count->get_int());
}

std::vector<std::int64_t> Database::user_list() const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $users = db_fetch_all('users');
    )"sv };

    const std::optional users{
        database::execute(path_, up::db_mode::OPEN_READONLY, jx9_prog)
            .transform(database::pair_to_vm)
            .and_then(std::bind(database::extract_variable, std::placeholders::_1, "users"s))
            .transform(database::value_to_ids)
    };

    if (!users.has_value()) {
        logging::error{ "Fail to fetch all users" };
        return {};
    }

    return users.value();
}

std::vector<std::int64_t> Database::admin_list() const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $admins = db_fetch_all('admins');
    )"sv };

    const std::optional admins{
        database::execute(path_, up::db_mode::OPEN_READONLY, jx9_prog)
            .transform(database::pair_to_vm)
            .and_then(std::bind(database::extract_variable, std::placeholders::_1, "admins"s))
            .transform(database::value_to_ids)
    };

    if (!admins.has_value()) {
        logging::error{ "Fail to fetch all admins" };
        return {};
    }

    return admins.value();
}

std::optional<std::int64_t> Database::add_video(const std::string& title, const std::string& video) const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $success = db_store('videos', $video);
        $video_id = $video.__id;
    )"sv };

    const std::optional success{
        database::execute(path_, up::db_mode::OPEN_READWRITE, jx9_prog,
                          { { "video"s, up::value::object{
                                            { "title"s, title },
                                            { "video"s, reinterpret_cast<const void*>(video.data()) },
                                            { "video_size"s, static_cast<std::int64_t>(video.size()) } } } })
            .transform(database::pair_to_vm)
            .and_then(std::bind(database::extract_variables, std::placeholders::_1, std::vector{ "success"s, "video_id"s }))
    };

    if (!success.has_value() || success->size() != 2 || !(*success)[0].get_bool()) {
        logging::error{ "Fail to add video \"{}\"", title };
        return false;
    }

    return (*success)[1].get_int();
}

bool Database::add_video_rights(const std::int64_t& id, const std::vector<std::int64_t>& user_ids) const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $make_video_right_callback = function($user_id) {
            return { 'video_id': $id, 'user_id': $user_id };
        };
        $video_rights = array_map($make_video_right_callback, $user_ids);
        $success = db_store('video_rights', $video_rights);
    )"sv };

    const std::optional success{
        database::execute(path_, up::db_mode::OPEN_READWRITE, jx9_prog,
                          { { "id"s, id }, { "user_ids"s, database::ids_to_values(user_ids) } })
            .transform(database::pair_to_vm)
            .and_then(std::bind(database::extract_variable, std::placeholders::_1, "success"s))
    };

    if (!success.has_value()) {
        logging::error{ "Fail to add video rights \"{}\"", id };
        return false;
    }

    return success->get_bool();
}

bool Database::update_video_rights(const std::int64_t& id, const std::vector<std::int64_t>& user_ids) const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $video_records_callback = function($video) {
            return ($video.video_id = $id);
        };
        $video_rights = db_fetch_all('video_rights', $video_records_callback);

        $drop_success = TRUE;
        foreach($video_rights as $video_right) {
            $drop_success = $drop_success && db_drop_record('video_rights', $video_right.__id);
        }
    )"sv };

    const std::optional drop_success{
        database::execute(path_, up::db_mode::OPEN_READWRITE, jx9_prog, { { "id"s, id } })
            .transform(database::pair_to_vm)
            .and_then(std::bind(database::extract_variable, std::placeholders::_1, "drop_success"s))
    };

    if (!drop_success.has_value()) {
        logging::error{ "Fail to delete video rights \"{}\"", id };
        return false;
    }

    if (!drop_success->get_bool()) {
        logging::error{ "Fail to delete video rights \"{}\"", id };
        return false;
    }

    return add_video_rights(id, user_ids);
}

bool Database::delete_video(const std::int64_t& id) const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $success = db_drop_record('videos', $id);
    )"sv };

    const std::optional success{
        database::execute(path_, up::db_mode::OPEN_READWRITE, jx9_prog, { { "id"s, id } })
            .transform(database::pair_to_vm)
            .and_then(std::bind(database::extract_variable, std::placeholders::_1, "success"s))
    };

    if (!success.has_value()) {
        logging::error{ "Fail to delete video \"{}\"", id };
        return false;
    }

    return success->get_bool();
}

bool Database::increment_video_views(const std::int64_t& id) const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $video = db_fetch_by_id('videos', $id);
        $drop_success = db_drop_record('videos', $id);
        if ($drop_success) {
            $video.views = $videos.views + 1;
            $store_success = db_store('videos', $video);
        }
    )"sv };

    const std::optional success{
        database::execute(path_, up::db_mode::OPEN_READWRITE, jx9_prog, { { "id"s, id } })
            .transform(database::pair_to_vm)
            .and_then(std::bind(database::extract_variables, std::placeholders::_1, std::vector{ "drop_success"s, "store_success"s }))
    };

    if (!success.has_value() || success->size() != 2) {
        logging::error{ "Fail to increment video views \"{}\"", id };
        return false;
    }

    return ((*success)[0].get_bool() && (*success)[1].get_bool());
}

bool Database::has_video_right(const std::int64_t& id) const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $video_right_callback = function($right) {
            return ($right.video_id == $id);
        };
        $video_rights = db_fetch_all('video_rights', $video_right_callback);
        $has_video_right = (count($video_rights == 0);
    )"sv };

    const std::optional has_video_right{
        database::execute(path_, up::db_mode::OPEN_READONLY, jx9_prog, { { "id"s, id } })
            .transform(database::pair_to_vm)
            .and_then(std::bind(database::extract_variable, std::placeholders::_1, "has_video_right"s))
    };

    if (!has_video_right.has_value()) {
        logging::error{ "Fail to get has video right \"{}\"", id };
        return {};
    }

    return has_video_right->get_bool();
}

bool Database::has_video_right(const std::int64_t& id, const std::int64_t& user_id) const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $video_right_callback = function($right) {
            return ($right.video_id == $id && $right.user_id = $user_id);
        };
        $video_rights = db_fetch_all('video_rights', $video_right_callback);
        $has_video_right = (count($video_rights != 0);
    )"sv };

    const std::optional has_video_right{
        database::execute(path_, up::db_mode::OPEN_READONLY, jx9_prog, { { "id"s, id }, { "user_id", user_id } })
            .transform(database::pair_to_vm)
            .and_then(std::bind(database::extract_variable, std::placeholders::_1, "has_video_right"s))
    };

    if (!has_video_right.has_value()) {
        logging::error{ "Fail to get has video right \"{}\"", id };
        return {};
    }

    return has_video_right->get_bool();
}

std::vector<std::int64_t> Database::video_right_list(const std::int64_t& id) const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $video_right_callback = function($right) {
            return ($right.video_id == $id);
        };
        $video_rights = db_fetch_all('video_rights', $video_right_callback);
    )"sv };

    const std::optional video_rights{
        database::execute(path_, up::db_mode::OPEN_READONLY, jx9_prog, { { "id"s, id } })
            .transform(database::pair_to_vm)
            .and_then(std::bind(database::extract_variable, std::placeholders::_1, "video_rights"s))
            .transform(database::value_to_ids)
    };

    if (!video_rights.has_value()) {
        logging::error{ "Fail to get video rights \"{}\"", id };
        return {};
    }

    return video_rights.value();
}

inline std::optional<std::pair<up::db, up::vm>> database::execute(const std::filesystem::path& path, up::db_mode mode, const std::string_view& jx9_program) noexcept
{
    return execute(path, mode, jx9_program, {});
}

inline std::optional<std::pair<up::db, up::vm>> database::execute(const std::filesystem::path& path, up::db_mode mode, const std::string_view& jx9_program,
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

inline up::vm database::pair_to_vm(std::pair<up::db, up::vm> pair) noexcept
{
    return std::move(pair.second);
}

inline std::optional<up::vm_value> database::extract_variable(up::vm vm, const std::string& record_name) noexcept
{
    return vm.extract(record_name);
}

inline std::optional<std::vector<up::vm_value>> database::extract_variables(up::vm vm, const std::vector<std::string>& record_names) noexcept
{
    std::vector<up::vm_value> values;
    values.reserve(record_names.size());
    for (const std::string& record_name : record_names) {
        std::optional value{ vm.extract(record_name) };
        if (value.has_value()) {
            values.emplace_back(std::move(value.value()));
        }
    }

    if (values.size() != record_names.size())
        return std::nullopt;

    return values;
}

inline std::optional<up::vm_value> database::find_member(const up::vm_value& value, const std::string& member_name) noexcept
{
    return value.find(member_name);
}

inline std::optional<std::vector<up::vm_value>> database::find_members(const up::vm_value& value, const std::vector<std::string>& member_names) noexcept
{
    std::vector<up::vm_value> members;
    members.reserve(member_names.size());
    for (const std::string& member_name : member_names) {
        std::optional member{ value.find(member_name) };
        if (member.has_value()) {
            members.emplace_back(std::move(member.value()));
        }
    }

    if (members.size() != member_names.size())
        return std::nullopt;

    return members;
}

inline std::vector<std::int64_t> database::value_to_ids(const up::vm_value& value) noexcept
{
    std::vector<std::int64_t> ids;
    const std::function id_callback{
        [&ids](const std::string& key, const up::vm_value& value) -> bool {
            if (key == "__id") {
                ids.push_back(value.get_int());
                return true;
            }
            return false;
        }
    };

    if (!value.foreach_object(id_callback)) {
        return {};
    }

    return ids;
}

inline std::vector<up::value> database::ids_to_values(const std::vector<std::int64_t>& ids) noexcept
{
    std::vector<up::value> values(ids.size());
    std::ranges::copy(ids, values.begin());
    return values;
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
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized" // gcc-alpine
extern "C" {
#include <unqlite-src/unqlite.c>
}
#pragma GCC diagnostic pop
