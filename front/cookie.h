#pragma once

#include <chrono>
#include <string>

namespace cookie
{
    using namespace std::chrono_literals;
    std::string insert_to_cookie([[maybe_unused]] const std::string& url, const std::string& key, const std::string& value, std::chrono::seconds max_age = 24h);
    std::string value_from_cookie(const std::string& cookie, const std::string& key);
}
