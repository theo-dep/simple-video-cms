#include "database.h"

#include "servercommon.h"

#include <mysql+++.h>

daotk::mysql::connect_options connect_options()
{
    daotk::mysql::connect_options options{
        sc::get_env("MYSQL_DB_URL", "localhost"),
        sc::get_env("MYSQL_ROOT_USER", "root"),
        sc::get_env("MYSQL_ROOT_PASSWORD", "1234"),
        sc::get_env("MYSQL_DB_NAME", "video")
    };
    options.port = std::stoi(sc::get_env("MYSQL_DB_PORT", "3306"));
    return options;
}

Database::Database()
{
}

std::vector<std::string> Database::most_viewed()
{
    daotk::mysql::connection conn;
    if (!conn.open(connect_options())) {
        ERR("Fail to open database connection");
        return {};
    }

    std::vector<std::string> ids;
    conn.query("SELECT video_ID FROM videos ORDER BY CAST(view_count as decimal) DESC LIMIT 10")
        .each([&ids](std::string id) {
            ids.push_back(id);
            return true;
        });

    return ids;
}
