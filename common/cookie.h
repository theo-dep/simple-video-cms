#pragma once

#include <chrono>
#include <string>

namespace cookie
{
    using namespace std::chrono_literals;
    static constexpr auto default_cookie_max_age = 24h;
    std::string insert_to_cookie(const std::string& key, const std::string& value, std::chrono::seconds max_age = default_cookie_max_age);
    std::string value_from_cookie(const std::string& cookie, const std::string& key);
}
