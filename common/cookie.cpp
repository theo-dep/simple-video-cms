#include "cookie.h"

#include "stringutils.h"

std::string cookie::insert_to_cookie([[maybe_unused]] const std::string& url, const std::string& key, const std::string& value, std::chrono::seconds max_age)
{
    const std::string same_site_secure{
        // #ifdef _DEBUG
        !url.contains("localhost") ? "SameSite=Strict;" :
                                   // #endif
            "SameSite=None; Secure;"
    };
    return {
        key + "=" + value + "; " +
        "; HttpOnly; Path=/;" + same_site_secure +
        "; " + "Max-Age=" + su::int_to_string(static_cast<int>(max_age.count()))
    };
}

std::string cookie::value_from_cookie(const std::string& cookie, const std::string& key)
{
    std::string value;
    std::size_t start{ cookie.find(key + '=') };
    if (start != std::string::npos) {
        start += key.length() + 1;
        std::size_t end{ cookie.find(';', start) };
        if (end == std::string::npos) {
            end = cookie.length();
        }
        value = cookie.substr(start, end - start);
    }

    return value;
}
