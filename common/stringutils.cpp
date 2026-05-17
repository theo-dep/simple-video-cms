#include "stringutils.h"

#include <algorithm>
#include <charconv>
#include <ranges>

std::vector<std::string> su::split(const std::string& str, char delim)
{
    const std::vector list(std::views::split(str, delim) | std::ranges::to<std::vector<std::string>>());
    return list;
}

void su::trim(std::string& str)
{
    static const auto ischar{
        [](unsigned char c) {
            return (std::isspace(c) == 0);
        }
    };
    // trim left
    str.erase(str.begin(), std::ranges::find_if(str, ischar));
    // trim right
    str.erase(std::ranges::find_if(std::views::reverse(str), ischar).base(), str.end());
}

void su::lower(std::string& str)
{
    static const auto tolower{
        [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        }
    };
    std::ranges::copy(std::views::transform(str, tolower), str.begin());
}

int su::string_to_int(const std::string& str)
{
    int value{ 0 };
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic): wait for c++26
    const std::from_chars_result ret{ std::from_chars(str.data(), str.data() + str.size(), value) };
    if (ret.ec == std::errc{})
        return value;
    return 0;
}

std::string su::int_to_string(int val)
{
    return std::to_string(val);
}
