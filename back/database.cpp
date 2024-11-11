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
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized" // gcc-alpine
#include <unqlitepp.hpp>
#pragma GCC diagnostic pop

#include <fstream>
#include <ranges>

namespace database
{
    enum user_type : std::int64_t
    {
        USER,
        ADMIN,
        SUPER_ADMIN
    };

    struct Data
    {
        Data(up::db&& db, up::vm&& vm) noexcept;
        Data(Data&& data) noexcept;
        up::db db;
        up::vm vm;
    };
    struct DataValue : Data
    {
        DataValue(Data&& data, up::value&& value) noexcept;
        DataValue(DataValue&& data) noexcept;
        up::value value;
    };
    struct DataValues : Data
    {
        DataValues(Data&& data, up::value::object&& values) noexcept;
        up::value::object values;
    };
    struct DataValueMember : DataValue
    {
        DataValueMember(DataValue&& data, up::value&& member) noexcept;
        up::value member;
    };
    struct DataValueMembers : DataValue
    {
        DataValueMembers(DataValue&& data, up::value::object&& members) noexcept;
        up::value::object members;
    };

    template <class F, typename... Args>
    auto bind(F&& f, Args&&... args) noexcept;

    std::string video_key(const std::int64_t& id) noexcept;

    std::optional<up::db> open(const std::filesystem::path& path, up::db_mode mode) noexcept;
    std::optional<Data> compile(up::db db, const std::string_view& jx9_program) noexcept;

    std::optional<Data> bind_variables(Data data, const up::value::object& binding_map) noexcept;
    std::optional<Data> execute(Data data) noexcept;

    std::optional<DataValue> extract_variable(Data data, const std::string& record_name) noexcept;
    std::optional<DataValues> extract_variables(Data data, const std::vector<std::string>& record_names) noexcept;
    std::optional<DataValueMember> find_member(DataValue data, const std::string& member_name) noexcept;
    std::optional<DataValueMembers> find_members(DataValue data, const std::vector<std::string>& member_names) noexcept;
    std::vector<up::value> values_to_members(DataValue data, const std::string& member_name) noexcept;
    std::vector<std::int64_t> values_to_id_members(DataValue data) noexcept;
    std::vector<up::value> ids_to_values(const std::vector<std::int64_t>& ids) noexcept;
    std::vector<std::int64_t> values_to_ids(const std::vector<up::value>& values) noexcept;
}

template <class F, typename... Args>
inline auto database::bind(F&& f, Args&&... args) noexcept
{
    return std::bind(std::forward<F>(f), std::placeholders::_1, std::forward<Args>(args)...);
}

Database::Database(const std::filesystem::path& path, bool& create_ok) noexcept
    : path_{ path }
{
    create_ok = true;
    if (!std::filesystem::exists(path_)) {
        std::ofstream{ path_ }; // create regular file
        const std::optional db{ database::open(path, up::db_mode::OPEN_CREATE) };
        create_ok = db.has_value();
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
                    salt: 'string',
                    type: 'integer'
                };
                db_set_schema('users', $schema);
            }
            if (!db_exists('videos')) {
                $rc = db_create('videos');
                if (!$rc) {
                    print db_errlog();
                    return;
                }

                $schema =  {
                    title: 'string',
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

    const std::optional data{
        database::open(path_, up::db_mode::OPEN_READWRITE)
            .and_then(database::bind(database::compile, jx9_prog))
            .and_then(database::execute)
    };
    return data.has_value();
}

std::vector<std::int64_t> Database::video_list() const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $videos = db_fetch_all('videos');
    )"sv };

    const std::optional videos{
        database::open(path_, up::db_mode::OPEN_READONLY)
            .and_then(database::bind(database::compile, jx9_prog))
            .and_then(database::execute)
            .and_then(database::bind(database::extract_variable, "videos"s))
            .transform(database::values_to_id_members)
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
            $found = FALSE;
            db_reset_record_cursor('video_rights');
            while (($video_right = db_fetch('video_rights')) != NULL && !$found) {
                $found = ($video_right.video_id == $video.__id && $video_right.user_id == $user_id);
            }
            return $found;
        };

        $videos = db_fetch_all('videos', $callback);
    )"sv };

    const std::optional videos{
        database::open(path_, up::db_mode::OPEN_READONLY)
            .and_then(database::bind(database::compile, jx9_prog))
            .and_then(database::bind(database::bind_variables, up::value::object{ { "user_id"s, user_id } }))
            .and_then(database::execute)
            .and_then(database::bind(database::extract_variable, "videos"s))
            .transform(database::values_to_id_members)
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
        $callback = function($video) {
            $found = FALSE;
            db_reset_record_cursor('video_rights');
            while (($video_right = db_fetch('video_rights')) != NULL && !$found) {
                $found = ($video_right.video_id == $video.__id);
            }
            return !$found;
        };

        $videos = db_fetch_all('videos', $callback);
    )"sv };

    const std::optional videos{
        database::open(path_, up::db_mode::OPEN_READONLY)
            .and_then(database::bind(database::compile, jx9_prog))
            .and_then(database::execute)
            .and_then(database::bind(database::extract_variable, "videos"s))
            .transform(database::values_to_id_members)
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
        database::open(path_, up::db_mode::OPEN_READONLY)
            .and_then(database::bind(database::compile, jx9_prog))
            .and_then(database::bind(database::bind_variables, up::value::object{ { "id"s, id } }))
            .and_then(database::execute)
            .and_then(database::bind(database::extract_variable, "video"s))
            .and_then(database::bind(database::find_member, "title"s))
    };

    if (!title.has_value()) {
        logging::error{ "Fail to fetch video title \"{}\"", id };
        return {};
    }

    return title->member.get_string();
}

std::int64_t Database::video_views(const std::int64_t& id) const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $video = db_fetch_by_id('videos', $id);
    )"sv };

    const std::optional views{
        database::open(path_, up::db_mode::OPEN_READONLY)
            .and_then(database::bind(database::compile, jx9_prog))
            .and_then(database::bind(database::bind_variables, up::value::object{ { "id"s, id } }))
            .and_then(database::execute)
            .and_then(database::bind(database::extract_variable, "video"s))
            .and_then(database::bind(database::find_member, "views"s))
    };

    if (!views.has_value()) {
        logging::error{ "Fail to fetch video views \"{}\"", id };
        return 0;
    }

    return views->member.get_int();
}

std::string Database::video(const std::int64_t& id) const noexcept
{
    up::db_kv_read_status status{ up::db_kv_read_status::OK };
    const std::optional video{
        database::open(path_, up::db_mode::OPEN_READONLY)
            .and_then([&id, &status](up::db db) -> std::optional<std::string> {
                std::string res;
                if (db.fetch_callback(
                        database::video_key(id),
                        [&res](const void* data, std::size_t size) {
                            res.append(static_cast<const char*>(data), size);
                            return true;
                        },
                        &status)) {
                    return res;
                } else {
                    return std::nullopt;
                }
            })
    };

    if (!video.has_value()) {
        logging::error{ "Fail to fetch video \"{}\"", id };
        if (status != up::db_kv_read_status::OK)
            logging::error{ "{}: {}", up::status_to_string_view<up::db_kv_read_status>::value, static_cast<std::size_t>(status) };
        return {};
    }

    return video.value();
}

std::optional<std::int64_t> Database::add_super_admin(const std::string& name, const std::string& password, const std::string& salt) const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $success = db_store('users', $admin);
        $admin_id = $admin.__id;
    )"sv };

    std::optional success{
        database::open(path_, up::db_mode::OPEN_READWRITE)
            .and_then(database::bind(database::compile, jx9_prog))
            .and_then(std::bind(database::bind_variables, std::placeholders::_1,
                                up::value::object{ { "admin"s,
                                                     up::value::object{
                                                         { "name"s, name },
                                                         { "password"s, password },
                                                         { "salt"s, salt },
                                                         { "type"s, database::user_type::SUPER_ADMIN } } } }))
            .and_then(database::execute)
            .and_then(database::bind(database::extract_variables, std::vector{ "success"s, "admin_id"s }))
    };

    if (!success.has_value() || success->values.size() != 2 || !success->values["success"s].get_bool()) {
        logging::error{ "Fail to add super admin \"{}\"", name };
        return std::nullopt;
    }

    return success->values["admin_id"s].get_int();
}

bool Database::is_super_admin(const std::int64_t& id) const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $admin = db_fetch_by_id('users', $id);
    )"sv };

    const std::optional user_type{
        database::open(path_, up::db_mode::OPEN_READONLY)
            .and_then(database::bind(database::compile, jx9_prog))
            .and_then(database::bind(database::bind_variables, up::value::object{ { "id"s, id } }))
            .and_then(database::execute)
            .and_then(database::bind(database::extract_variable, "admin"s))
            .and_then(database::bind(database::find_member, "type"s))
    };

    if (!user_type.has_value()) {
        logging::error{ "Fail to get super admin \"{}\"", id };
        return false;
    }

    return (user_type->member.get_int() == database::user_type::SUPER_ADMIN);
}

bool Database::is_admin(const std::int64_t& id) const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $admin = db_fetch_by_id('users', $id);
    )"sv };

    const std::optional user_type{
        database::open(path_, up::db_mode::OPEN_READONLY)
            .and_then(database::bind(database::compile, jx9_prog))
            .and_then(database::bind(database::bind_variables, up::value::object{ { "id"s, id } }))
            .and_then(database::execute)
            .and_then(database::bind(database::extract_variable, "admin"s))
            .and_then(database::bind(database::find_member, "type"s))
    };

    if (!user_type.has_value()) {
        logging::error{ "Fail to get admin \"{}\"", id };
        return false;
    }

    return (user_type->member.get_int() != database::user_type::USER);
}

bool Database::is_user(const std::int64_t& id) const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $user = db_fetch_by_id('users', $id);
    )"sv };

    const std::optional user_type{
        database::open(path_, up::db_mode::OPEN_READONLY)
            .and_then(database::bind(database::compile, jx9_prog))
            .and_then(database::bind(database::bind_variables, up::value::object{ { "id"s, id } }))
            .and_then(database::execute)
            .and_then(database::bind(database::extract_variable, "user"s))
            .and_then(database::bind(database::find_member, "type"s))
    };

    if (!user_type.has_value()) {
        logging::error{ "Fail to get user \"{}\"", id };
        return false;
    }

    return (user_type->member.get_int() == database::user_type::USER);
}

std::optional<std::int64_t> Database::add_admin(const std::string& name, const std::string& password, const std::string& salt) const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $success = db_store('users', $admin);
        $admin_id = $admin.__id;
    )"sv };

    std::optional success{
        database::open(path_, up::db_mode::OPEN_READWRITE)
            .and_then(database::bind(database::compile, jx9_prog))
            .and_then(std::bind(database::bind_variables, std::placeholders::_1,
                                up::value::object{ { "admin"s,
                                                     up::value::object{
                                                         { "name"s, name },
                                                         { "password"s, password },
                                                         { "salt"s, salt },
                                                         { "type"s, database::user_type::ADMIN } } } }))
            .and_then(database::execute)
            .and_then(database::bind(database::extract_variables, std::vector{ "success"s, "admin_id"s }))
    };

    if (!success.has_value() || success->values.size() != 2 || !success->values["success"s].get_bool()) {
        logging::error{ "Fail to add admin \"{}\"", name };
        return std::nullopt;
    }

    return success->values["admin_id"s].get_int();
}

std::optional<std::int64_t> Database::add_user(const std::string& name, const std::string& password, const std::string& salt) const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $success = db_store('users', $user);
        $user_id = $user.__id;
    )"sv };

    std::optional success{
        database::open(path_, up::db_mode::OPEN_READWRITE)
            .and_then(database::bind(database::compile, jx9_prog))
            .and_then(std::bind(database::bind_variables, std::placeholders::_1,
                                up::value::object{ { "user"s, up::value::object{
                                                                  { "name"s, name },
                                                                  { "password"s, password },
                                                                  { "salt"s, salt },
                                                                  { "type"s, database::user_type::USER } } } }))
            .and_then(database::execute)
            .and_then(database::bind(database::extract_variables, std::vector{ "success"s, "user_id"s }))
    };

    if (!success.has_value() || success->values.size() != 2 || !success->values["success"s].get_bool()) {
        logging::error{ "Fail to add user \"{}\"", name };
        return std::nullopt;
    }

    return success->values["user_id"s].get_int();
}

bool Database::update_user(const std::int64_t& id, const std::string& password) const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $user = db_fetch_by_id('users', $id);
        $user.password = $password;
        $success = db_update_record('users', $id, $user);
    )"sv };

    std::optional success{
        database::open(path_, up::db_mode::OPEN_READWRITE)
            .and_then(database::bind(database::compile, jx9_prog))
            .and_then(database::bind(database::bind_variables, up::value::object{ { "id"s, id }, { "password"s, password } }))
            .and_then(database::execute)
            .and_then(database::bind(database::extract_variable, "success"s))
    };

    if (!success.has_value()) {
        logging::error{ "Fail to update admin \"{}\"", id };
        return false;
    }

    return success->value.get_bool();
}

bool Database::delete_user(const std::int64_t& id) const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $success = db_drop_record('users', $id);
    )"sv };

    const std::optional success{
        database::open(path_, up::db_mode::OPEN_READWRITE)
            .and_then(database::bind(database::compile, jx9_prog))
            .and_then(database::bind(database::bind_variables, up::value::object{ { "id"s, id } }))
            .and_then(database::execute)
            .and_then(database::bind(database::extract_variable, "success"s))
    };

    if (!success.has_value()) {
        logging::error{ "Fail to delete user \"{}\"", id };
        return false;
    }

    return success->value.get_bool();
}

std::int64_t Database::user_id(const std::string& name) const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $callback = function($user) {
            return ($user.name == $username);
        };
        $users = db_fetch_all('users', $callback);
    )"sv };

    const std::optional ids{
        database::open(path_, up::db_mode::OPEN_READONLY)
            .and_then(database::bind(database::compile, jx9_prog))
            .and_then(database::bind(database::bind_variables, up::value::object{ { "username"s, name } }))
            .and_then(database::execute)
            .and_then(database::bind(database::extract_variable, "users"s))
            .transform(database::values_to_id_members)
    };

    if (!ids.has_value() || ids->empty()) {
        logging::debug{ "Fail to fetch user \"{}\"", name }; // debug not error because user may not exist
        return -1;
    }

    return (*ids)[0];
}

std::string Database::user_name(const std::int64_t& id) const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $user = db_fetch_by_id('users', $id);
    )"sv };

    const std::optional name{
        database::open(path_, up::db_mode::OPEN_READONLY)
            .and_then(database::bind(database::compile, jx9_prog))
            .and_then(database::bind(database::bind_variables, up::value::object{ { "id"s, id } }))
            .and_then(database::execute)
            .and_then(database::bind(database::extract_variable, "user"s))
            .and_then(database::bind(database::find_member, "name"s))
    };

    if (!name.has_value()) {
        logging::error{ "Fail to fetch user name \"{}\"", id };
        return {};
    }

    return name->member.get_string();
}

std::string Database::user_password(const std::int64_t& id) const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $user = db_fetch_by_id('users', $id);
    )"sv };

    const std::optional password{
        database::open(path_, up::db_mode::OPEN_READONLY)
            .and_then(database::bind(database::compile, jx9_prog))
            .and_then(database::bind(database::bind_variables, up::value::object{ { "id"s, id } }))
            .and_then(database::execute)
            .and_then(database::bind(database::extract_variable, "user"s))
            .and_then(database::bind(database::find_member, "password"s))
    };

    if (!password.has_value()) {
        logging::error{ "Fail to fetch user password \"{}\"", id };
        return {};
    }

    return password->member.get_string();
}

std::string Database::user_salt(const std::int64_t& id) const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $user = db_fetch_by_id('users', $id);
    )"sv };

    const std::optional salt{
        database::open(path_, up::db_mode::OPEN_READONLY)
            .and_then(database::bind(database::compile, jx9_prog))
            .and_then(database::bind(database::bind_variables, up::value::object{ { "id"s, id } }))
            .and_then(database::execute)
            .and_then(database::bind(database::extract_variable, "user"s))
            .and_then(database::bind(database::find_member, "salt"s))
    };

    if (!salt.has_value()) {
        logging::error{ "Fail to fetch user salt \"{}\"", id };
        return {};
    }

    return salt->member.get_string();
}

std::int64_t Database::user_count() const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $count = db_total_records('users');
    )"sv };

    const std::optional count{
        database::open(path_, up::db_mode::OPEN_READONLY)
            .and_then(database::bind(database::compile, jx9_prog))
            .and_then(database::execute)
            .and_then(database::bind(database::extract_variable, "count"s))
    };

    if (!count.has_value()) {
        logging::error{ "Fail to get user count" };
        return 0;
    }

    return count->value.get_int();
}

std::int64_t Database::video_count() const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $count = db_total_records('videos');
    )"sv };

    const std::optional count{
        database::open(path_, up::db_mode::OPEN_READONLY)
            .and_then(database::bind(database::compile, jx9_prog))
            .and_then(database::execute)
            .and_then(database::bind(database::extract_variable, "count"s))
    };

    if (!count.has_value()) {
        logging::error{ "Fail to get video count" };
        return 0;
    }

    return count->value.get_int();
}

std::int64_t Database::view_count() const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $videos = db_fetch_all('videos');
        $views_callback = function($video) {
            return $video.views;
        };
        $count = array_sum(array_map($views_callback, $videos));
    )"sv };

    const std::optional count{
        database::open(path_, up::db_mode::OPEN_READONLY)
            .and_then(database::bind(database::compile, jx9_prog))
            .and_then(database::execute)
            .and_then(database::bind(database::extract_variable, "count"s))
    };

    if (!count.has_value()) {
        logging::error{ "Fail to get view count" };
        return 0;
    }

    return count->value.get_int();
}

std::vector<std::int64_t> Database::user_list() const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $callback = function($user) {
            return ($user.type == $user_type);
        };
        $users = db_fetch_all('users', $callback);
    )"sv };

    const std::optional users{
        database::open(path_, up::db_mode::OPEN_READONLY)
            .and_then(database::bind(database::compile, jx9_prog))
            .and_then(database::bind(database::bind_variables, up::value::object{ { "user_type"s, database::user_type::USER } }))
            .and_then(database::execute)
            .and_then(database::bind(database::extract_variable, "users"s))
            .transform(database::values_to_id_members)
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
        $callback = function($user) {
            return ($user.type == $admin_type || $user.type == $super_admin_type);
        };
        $admins = db_fetch_all('users', $callback);
    )"sv };

    const std::optional admins{
        database::open(path_, up::db_mode::OPEN_READONLY)
            .and_then(database::bind(database::compile, jx9_prog))
            .and_then(std::bind(database::bind_variables, std::placeholders::_1,
                                up::value::object{
                                    { "admin_type"s, database::user_type::ADMIN }, { "super_admin_type"s, database::user_type::SUPER_ADMIN } }))
            .and_then(database::execute)
            .and_then(database::bind(database::extract_variable, "admins"s))
            .transform(database::values_to_id_members)
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

    std::optional success{
        database::open(path_, up::db_mode::OPEN_READWRITE)
            .and_then(database::bind(database::compile, jx9_prog))
            .and_then(std::bind(database::bind_variables, std::placeholders::_1,
                                up::value::object{ { "video"s,
                                                     up::value::object{
                                                         { "title"s, title },
                                                         { "views"s, std::int64_t{ 0 } } } } }))
            .and_then(database::execute)
            .and_then(database::bind(database::extract_variables, std::vector{ "success"s, "video_id"s }))
    };

    if (!success.has_value() || success->values.size() != 2 || !success->values["success"s].get_bool()) {
        logging::error{ "Fail to add video \"{}\"", title };
        return std::nullopt;
    }

    const std::int64_t video_id{ success->values["video_id"s].get_int() };

    // store video in kv space to speed fetching of "videos" table
    up::db_kv_write_status status;
    std::string_view error_text;
    if (!success->db.store(database::video_key(video_id), video, &status, &error_text)) {
        logging::error{ "Fail to add video \"{}\"", video_id };
        logging::error{ "{}: {}. {}",
                        up::status_to_string_view<up::db_kv_write_status>::value, static_cast<std::size_t>(status), error_text };
        return std::nullopt;
    }

    return video_id;
}

bool Database::add_video_rights(const std::int64_t& id, const std::vector<std::int64_t>& user_ids) const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $make_video_right_callback = function($user_id) {
            return { video_id: $id, user_id: $user_id };
        };
        $video_rights = array_map($make_video_right_callback, $user_ids);
        $success = db_store('video_rights', $video_rights);
    )"sv };

    const std::optional success{
        database::open(path_, up::db_mode::OPEN_READWRITE)
            .and_then(database::bind(database::compile, jx9_prog))
            .and_then(std::bind(database::bind_variables, std::placeholders::_1,
                                up::value::object{ { "id"s, id }, { "user_ids"s, database::ids_to_values(user_ids) } }))
            .and_then(database::execute)
            .and_then(database::bind(database::extract_variable, "success"s))
    };

    if (!success.has_value()) {
        logging::error{ "Fail to add video rights \"{}\"", id };
        return false;
    }

    return success->value.get_bool();
}

namespace database
{
    bool delete_video_rights(const std::filesystem::path& path, const std::int64_t& id) noexcept;
}

bool database::delete_video_rights(const std::filesystem::path& path, const std::int64_t& id) noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $success = TRUE;
        db_reset_record_cursor('video_rights');
        while (($video_right = db_fetch('video_rights')) != NULL) {
            if ($video_right.video_id == $id) {
                $success = $success && db_drop_record('video_rights', $video_right.__id);
            }
        }
    )"sv };

    const std::optional success{
        database::open(path, up::db_mode::OPEN_READWRITE)
            .and_then(database::bind(database::compile, jx9_prog))
            .and_then(database::bind(database::bind_variables, up::value::object{ { "id"s, id } }))
            .and_then(database::execute)
            .and_then(database::bind(database::extract_variable, "success"s))
    };

    if (!success.has_value()) {
        logging::error{ "Fail to delete video rights \"{}\"", id };
        return false;
    }

    return success->value.get_bool();
}

bool Database::update_video_rights(const std::int64_t& id, const std::vector<std::int64_t>& user_ids) const noexcept
{
    if (!database::delete_video_rights(path_, id)) {
        logging::error{ "Fail to delete video rights \"{}\"", id };
        return false;
    }

    return add_video_rights(id, user_ids);
}

bool Database::delete_video(const std::int64_t& id) const noexcept
{
    if (!database::delete_video_rights(path_, id)) {
        logging::error{ "Fail to delete video rights \"{}\"", id };
        return false;
    }

    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $success = db_drop_record('videos', $id);
    )"sv };

    std::optional success{
        database::open(path_, up::db_mode::OPEN_READWRITE)
            .and_then(database::bind(database::compile, jx9_prog))
            .and_then(database::bind(database::bind_variables, up::value::object{ { "id"s, id } }))
            .and_then(database::execute)
            .and_then(database::bind(database::extract_variable, "success"s))
    };

    if (!success.has_value()) {
        logging::error{ "Fail to delete video \"{}\"", id };
        return false;
    }

    up::db_kv_write_status status;
    std::string_view error_text;
    if (!success->db.remove(database::video_key(id), &status, &error_text)) {
        logging::error{ "Fail to delete video \"{}\"", id };
        logging::error{ "{}: {}. {}",
                        up::status_to_string_view<up::db_kv_write_status>::value, static_cast<std::size_t>(status), error_text };
        return false;
    }

    return success->value.get_bool();
}

bool Database::increment_video_views(const std::int64_t& id) const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $video = db_fetch_by_id('videos', $id);
        $video.views += 1;
        $success = db_update_record('videos', $id, $video);
    )"sv };

    std::optional success{
        database::open(path_, up::db_mode::OPEN_READWRITE)
            .and_then(database::bind(database::compile, jx9_prog))
            .and_then(database::bind(database::bind_variables, up::value::object{ { "id"s, id } }))
            .and_then(database::execute)
            .and_then(database::bind(database::extract_variable, "success"s))
    };

    if (!success.has_value()) {
        logging::error{ "Fail to increment video views \"{}\"", id };
        return false;
    }

    return success->value.get_bool();
}

bool Database::has_video_right(const std::int64_t& id) const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $video_right_callback = function($right) {
            return ($right.video_id == $id);
        };
        $video_rights = db_fetch_all('video_rights', $video_right_callback);
        $has_video_right = (count($video_rights) == 0);
    )"sv };

    const std::optional has_video_right{
        database::open(path_, up::db_mode::OPEN_READONLY)
            .and_then(database::bind(database::compile, jx9_prog))
            .and_then(database::bind(database::bind_variables, up::value::object{ { "id"s, id } }))
            .and_then(database::execute)
            .and_then(database::bind(database::extract_variable, "has_video_right"s))
    };

    if (!has_video_right.has_value()) {
        logging::error{ "Fail to get has video right \"{}\"", id };
        return {};
    }

    return has_video_right->value.get_bool();
}

bool Database::has_video_right(const std::int64_t& id, const std::int64_t& user_id) const noexcept
{
    using namespace std::literals;
    constexpr std::string_view jx9_prog{ R"(
        $user = db_fetch_by_id('users', $user_id);
        $user_type = $user.type;

        $video_right_callback = function($right) {
            return ($right.video_id == $id && $right.user_id = $user_id);
        };
        $video_rights = db_fetch_all('video_rights', $video_right_callback);
        $has_video_right = (count($video_rights) != 0);
    )"sv };

    std::optional has_video_right{
        database::open(path_, up::db_mode::OPEN_READONLY)
            .and_then(database::bind(database::compile, jx9_prog))
            .and_then(database::bind(database::bind_variables, up::value::object{ { "id"s, id }, { "user_id", user_id } }))
            .and_then(database::execute)
            .and_then(database::bind(database::extract_variables, std::vector{ "user_type"s, "has_video_right"s }))
    };

    if (!has_video_right.has_value()) {
        logging::error{ "Fail to get has video right \"{}\"", id };
        return {};
    }

    // check if user is admin
    if (has_video_right->values["user_type"].get_int() != database::user_type::USER)
        return true;

    return has_video_right->values["has_video_right"].get_bool();
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
        database::open(path_, up::db_mode::OPEN_READONLY)
            .and_then(database::bind(database::compile, jx9_prog))
            .and_then(database::bind(database::bind_variables, up::value::object{ { "id"s, id } }))
            .and_then(database::execute)
            .and_then(database::bind(database::extract_variable, "video_rights"s))
            .transform(database::bind(database::values_to_members, "user_id"s))
            .transform(database::values_to_ids)
    };

    if (!video_rights.has_value()) {
        logging::error{ "Fail to get video rights \"{}\"", id };
        return {};
    }

    return video_rights.value();
}

inline database::Data::Data(up::db&& db, up::vm&& vm) noexcept
    : db{ std::move(db) }
    , vm{ std::move(vm) }
{
}

inline database::Data::Data(Data&& data) noexcept
    : db{ std::move(data.db) }
    , vm{ std::move(data.vm) }
{
}

inline database::DataValue::DataValue(Data&& data, up::value&& value) noexcept
    : Data(std::move(data))
    , value{ std::move(value) }
{
}

inline database::DataValue::DataValue(DataValue&& data) noexcept
    : Data(std::move(data))
    , value{ std::move(data.value) }
{
}

inline database::DataValues::DataValues(Data&& data, up::value::object&& values) noexcept
    : Data(std::move(data))
    , values{ std::move(values) }
{
}

inline database::DataValueMember::DataValueMember(DataValue&& data, up::value&& member) noexcept
    : DataValue(std::move(data))
    , member{ std::move(member) }
{
}

inline database::DataValueMembers::DataValueMembers(DataValue&& data, up::value::object&& members) noexcept
    : DataValue(std::move(data))
    , members{ std::move(members) }
{
}

inline std::string database::video_key(const std::int64_t& id) noexcept
{
    return "video_" + su::int_to_string(id);
}

inline std::optional<up::db> database::open(const std::filesystem::path& path, up::db_mode mode) noexcept
{
    up::db_make_status make_status;
    std::optional db{ up::db::make(path, mode, &make_status) };
    if (!db.has_value()) {
        logging::error{ "{}: {}", up::status_to_string_view<up::db_make_status>::value, static_cast<std::size_t>(make_status) };
        return std::nullopt;
    }
    return db;
}

inline std::optional<database::Data> database::compile(up::db db, const std::string_view& jx9_program) noexcept
{
    up::db_compilation_status compile_status;
    std::string_view compile_error;
    std::optional vm{ db.compile(jx9_program, &compile_status, &compile_error) };
    if (!vm.has_value()) {
        logging::error{ "{}: {}. {}",
                        up::status_to_string_view<up::db_compilation_status>::value, static_cast<std::size_t>(compile_status), compile_error };
        return std::nullopt;
    }
    return Data{ std::move(db), std::move(vm.value()) };
}

inline std::optional<database::Data> database::bind_variables(Data data, const up::value::object& binding_map) noexcept
{
    for (const auto& [key, value] : binding_map) {
        if (!data.vm.bind(key, value)) {
            logging::error{ "Fail to bind key: {}", key };
            return std::nullopt;
        }
    }
    return data;
}

inline std::optional<database::Data> database::execute(Data data) noexcept
{
    up::vm_execute_status exe_status;
    if (!data.vm.exec(&exe_status)) {
        logging::error{ "{}: {}", up::status_to_string_view<up::vm_execute_status>::value, static_cast<std::size_t>(exe_status) };
        return std::nullopt;
    }
    return data;
}

inline std::optional<database::DataValue> database::extract_variable(Data data, const std::string& record_name) noexcept
{
    std::optional value{ data.vm.extract(record_name) };
    if (!value.has_value())
        return std::nullopt;

    return DataValue{ std::move(data), std::move(value->make_value()) };
}

inline std::optional<database::DataValues> database::extract_variables(Data data, const std::vector<std::string>& record_names) noexcept
{
    up::value::object values;
    for (const std::string& record_name : record_names) {
        std::optional value{ data.vm.extract(record_name) };
        if (value.has_value()) {
            values.emplace(record_name, std::move(value->make_value()));
        }
    }

    if (values.size() != record_names.size())
        return std::nullopt;

    return DataValues{ std::move(data), std::move(values) };
}

inline std::optional<database::DataValueMember> database::find_member(DataValue data, const std::string& member_name) noexcept
{
    if (!data.value.is_object())
        return std::nullopt;

    // up::value* const member{ data.value.find(member_name) }; // FIXME: infinite recursion
    up::value member;
    // FIXME: always return false, to not test
    data.value.foreach_object([&member_name, &member](const std::string& key, const up::value& value) -> bool {
        if (key == member_name) {
            member = value;
        }
        return true; // false will abort
    });

    if (member.is_null())
        return std::nullopt;

    return DataValueMember{ std::move(data), std::move(member) };
}

inline std::optional<database::DataValueMembers> database::find_members(DataValue data, const std::vector<std::string>& member_names) noexcept
{
    if (!data.value.is_object())
        return std::nullopt;

    up::value::object members;
    // FIXME: always return false, to not test
    data.value.foreach_object([&member_names, &members](const std::string& key, const up::value& value) -> bool {
        if (std::ranges::find(member_names, key) != member_names.cend()) {
            members.emplace(key, value);
        }
        return true; // false will abort
    });

    // FIXME: infinite recursion
    // for (const std::string& member_name : member_names) {
    //    up::value* const member{ data.value.find(member_name) };
    //    if (member != nullptr) {
    //        members.emplace(member_name, std::move(*member));
    //    }
    //}

    if (members.size() != member_names.size())
        return std::nullopt;

    return DataValueMembers{ std::move(data), std::move(members) };
}

inline std::vector<up::value> database::values_to_members(DataValue data, const std::string& member_name) noexcept
{
    std::vector<up::value> ids;
    const std::function id_callback{
        [&ids, &member_name](std::size_t /*index*/, const up::value& value) -> bool {
            const up::value* const id{ value.find(member_name) };
            if (id != nullptr) {
                ids.emplace_back(std::move(*id));
            }
            return true; // false will abort
        }
    };

    data.value.foreach_array(id_callback); // FIXME: always return false, to not test
    return ids;
}

inline std::vector<std::int64_t> database::values_to_id_members(DataValue data) noexcept
{
    std::vector<std::int64_t> ids;
    const std::function id_callback{
        [&ids](std::size_t /*index*/, const up::value& value) -> bool {
            const up::value* const id{ value.find("__id") };
            if (id != nullptr) {
                ids.emplace_back(id->get_int());
            }
            return true; // false will abort
        }
    };

    data.value.foreach_array(id_callback); // FIXME: always return false, to not test
    return ids;
}

inline std::vector<up::value> database::ids_to_values(const std::vector<std::int64_t>& ids) noexcept
{
    std::vector<up::value> values(ids.size());
    std::ranges::copy(ids, values.begin());
    return values;
}

inline std::vector<std::int64_t> database::values_to_ids(const std::vector<up::value>& values) noexcept
{
    std::vector<std::int64_t> ids(values.size());
    std::ranges::transform(values, ids.begin(), std::bind(&up::value::get_int, std::placeholders::_1));
    return ids;
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
