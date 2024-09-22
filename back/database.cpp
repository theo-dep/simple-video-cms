#include "database.h"

#include "servercommon.h"

#include <mysql+++.h>

#include <format>

namespace database
{
    const daotk::mysql::connect_options& connect_options() noexcept;
}

#define BEGIN_QUERY try
#define END_QUERY                                                                                      \
    catch (const daotk::mysql::mysql_exception& exp)                                                   \
    {                                                                                                  \
        ERR(std::format("Query failed with error: {} ({})", exp.error_message(), exp.error_number())); \
    }                                                                                                  \
    catch (const daotk::mysql::mysqlpp_exception& exp)                                                 \
    {                                                                                                  \
        ERR(std::format("Query failed with error: {}", exp.error_message()));                          \
    }                                                                                                  \
    catch (const std::exception& exp)                                                                  \
    {                                                                                                  \
        ERR(std::format("Query failed with error: {}", exp.what()));                                   \
    }                                                                                                  \
    catch (...)                                                                                        \
    {                                                                                                  \
        ERR("Unknown fail");                                                                           \
    }

inline const daotk::mysql::connect_options& database::connect_options() noexcept
{
    static const daotk::mysql::connect_options options{
        []() -> daotk::mysql::connect_options {
            daotk::mysql::connect_options options{
                sc::get_env("MYSQL_DB_URL", "localhost"),
                sc::get_env("MYSQL_ROOT_USER", "root"),
                sc::get_env("MYSQL_ROOT_PASSWORD", "1234"),
                sc::get_env("MYSQL_DB_NAME", "video")
            };
            options.port = std::stoi(sc::get_env("MYSQL_DB_PORT", "3306"));
            options.charset = "utf8";
            return options;
        }()
    };
    return options;
}

std::vector<std::string> database::most_viewed() noexcept
{
    daotk::mysql::connection conn;
    if (!conn.open(connect_options())) {
        ERR("Fail to open database connection");
        return {};
    }

    std::vector<std::string> ids;
    BEGIN_QUERY
    {
        conn.query("SELECT video_ID FROM videos ORDER BY CAST(view_count as decimal) DESC LIMIT 10")
            .each([&ids](std::string id) {
                ids.push_back(id);
                return true;
            });
    }
    END_QUERY

    return ids;
}

bool database::is_valid_username(const std::string& username) noexcept
{
    daotk::mysql::connection conn;
    if (!conn.open(connect_options())) {
        ERR("Fail to open database connection");
        return false;
    }

    BEGIN_QUERY
    {
        return ((conn.query("SELECT COUNT(*) FROM users WHERE username = '%s'", username.c_str())
                     .get_value<int>() == 1) ||
                (conn.query("SELECT COUNT(*) FROM admins WHERE username = '%s'", username.c_str())
                     .get_value<int>() == 1));
    }
    END_QUERY

    return false;
}

void database::add_user(const std::string& username, const std::string& password) noexcept
{
    daotk::mysql::connection conn;
    if (!conn.open(connect_options())) {
        ERR("Fail to open database connection");
        return;
    }

    BEGIN_QUERY
    {
        conn.exec("INSERT INTO users VALUES ('%s', '%s')", username.c_str(), password.c_str());
    }
    END_QUERY
}

void database::add_admin(const std::string& username, const std::string& password) noexcept
{
    daotk::mysql::connection conn;
    if (!conn.open(connect_options())) {
        ERR("Fail to open database connection");
        return;
    }

    BEGIN_QUERY
    {
        conn.exec("INSERT INTO admins VALUES ('%s', '%s')", username.c_str(), password.c_str());
    }
    END_QUERY
}

std::string database::get_password(const std::string& username) noexcept
{
    daotk::mysql::connection conn;
    if (!conn.open(connect_options())) {
        ERR("Fail to open database connection");
        return {};
    }

    BEGIN_QUERY
    {
        return conn.query("SELECT password FROM users WHERE username = '%s'", username.c_str())
            .get_value<std::optional<std::string>>()
            .value_or(conn.query("SELECT password FROM admins WHERE username = '%s'", username.c_str())
                          .get_value<std::string>());
    }
    END_QUERY

    return {};
}

void database::update_session(const std::string& session_id, const std::string& username) noexcept
{
    daotk::mysql::connection conn;
    if (!conn.open(connect_options())) {
        ERR("Fail to open database connection");
        return;
    }

    (void)session_id;
    (void)username;
    // conn.exec("INSERT OR UPDATE INTO sessions VALUES(%s, %s)", session_id, username);
}
