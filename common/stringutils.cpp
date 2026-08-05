#include "stringutils.h"

#include <algorithm>
#include <charconv>
#include <ranges>

std::vector<std::string> su::split(const std::string& str, char delim)
{
    const std::vector list(std::views::split(str, delim) | std::ranges::to<std::vector<std::string>>());
    return list;
}

std::string su::trim(const std::string& str)
{
    static const auto ischar{
        [](unsigned char c) {
            return (std::isspace(c) == 0);
        }
    };
    std::string s{ str };
    // trim left
    s.erase(s.begin(), std::ranges::find_if(s, ischar));
    // trim right
    s.erase(std::ranges::find_if(std::views::reverse(s), ischar).base(), s.end());
    return s;
}

std::string su::lower(const std::string& str)
{
    static const auto tolower{
        [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        }
    };
    std::string s{ str };
    std::ranges::transform(s, s.begin(), tolower);
    return s;
}

bool su::string_to_bool(const std::string& str)
{
    return lower(str) == "true" || str == "1";
}

int su::string_to_int(const std::string& str)
{
    int value{ 0 };
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic): wait for c++26
    if (std::from_chars(str.data(), str.data() + str.size(), value))
        return value;
    return 0;
}

std::string su::int_to_string(int val)
{
    return std::to_string(val);
}

std::string su::time_point_to_string(std::chrono::system_clock::time_point tp)
{
    return std::format("{:%Y-%m-%dT%H:%M:%SZ}", std::chrono::floor<std::chrono::seconds>(tp));
}

std::optional<std::chrono::system_clock::time_point> su::string_to_time_point(const std::string& str)
{
    std::chrono::system_clock::time_point tp;
    std::istringstream ss(str);
    ss >> std::chrono::parse("%Y-%m-%dT%H:%M:%SZ", tp);
    return ss.fail() ? std::nullopt : std::optional{ tp };
}

std::string su::seconds_to_string(std::chrono::seconds d)
{
    return std::to_string(d.count());
}

std::chrono::seconds su::string_to_seconds(const std::string& str)
{
    std::int64_t value{ 0 };
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic): wait for c++26
    if (std::from_chars(str.data(), str.data() + str.size(), value))
        return std::chrono::seconds{ value };
    using namespace std::chrono_literals;
    return 0s;
}
