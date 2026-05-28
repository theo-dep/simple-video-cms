#include "cookie.h"

#include "stringutils.h"

#include <regex>

namespace cookie
{
    std::string cookie_key(const std::string& key)
    {
#ifndef _DEBUG
        return "__Host-" + key;
#else
        return key;
#endif
    }
}

std::string cookie::insert_to_cookie(const std::string& key, const std::string& value, std::chrono::seconds max_age)
{
    return {
        cookie_key(key) + "=" + value + "; " +
        "HttpOnly; Path=/; SameSite=Strict;" +
        "Max-Age=" + su::int_to_string(static_cast<int>(max_age.count()))
#ifndef _DEBUG
        + "; Secure;"
#endif
    };
}

std::string cookie::value_from_cookie(const std::string& cookie, const std::string& key)
{
    const std::regex cookie_regex(R"((?:^|;\s?))" + cookie_key(key) + R"(=([a-f0-9]+)(?:;|$))");
    std::smatch cookie_match;
    if (std::regex_search(cookie, cookie_match, cookie_regex)) {
        return cookie_match[1].str();
    }

    return {};
}
