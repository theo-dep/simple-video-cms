#include "stringutils.h"

#include <algorithm>
#include <ranges>

std::string su::join(const std::vector<std::string>& list, char delim) noexcept
{
    const std::string str{ std::ranges::fold_left(list | std::views::join_with(delim), std::string{}, std::plus{}) };
    return str;
}

std::vector<std::string> su::split(const std::string& str, char delim) noexcept
{
    // const std::vector<std::string> list{ std::ranges::split_view(str, delim) | std::ranges::to<std::vector>() };
    std::vector<std::string> list;
    for (auto&& item : std::views::split(str, delim)) {
        list.emplace_back(item.cbegin(), item.cend());
    }
    return list;
}

void su::trim(std::string& str) noexcept
{
    static const auto ischar{ [](const std::string::value_type& c) -> bool { return (std::isspace(c) == 0); } };
    // trim left
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), ischar));
    // trim right
    str.erase(std::ranges::find_if(std::views::reverse(str), ischar).base(), str.end());
}

void su::lower(std::string& str) noexcept
{
    static const auto tolower{
        [](const std::string::value_type& c) -> std::string::value_type {
            return static_cast<std::string::value_type>(std::tolower(c));
        }
    };
    std::ranges::copy(std::views::transform(str, tolower), str.begin());
}

std::string su::bool_to_string(bool b) noexcept
{
    return (b ? "true" : "false");
}

bool su::string_to_bool(const std::string& str) noexcept
{
    return (str == "true");
}

int su::string_to_int(const std::string& str, bool* ok) noexcept
{
    if (ok != nullptr)
        *ok = true;

    try {
        return std::stoi(str);
    } catch (...) {
        if (ok != nullptr)
            *ok = false;
        return 0;
    }
}

std::string su::md5_varchar_to_string(const types::md5_varchar& array) noexcept
{
    std::string str;
    str.resize(array.size());
    std::ranges::copy(array, str.begin());
    return str;
}

types::md5_varchar su::string_to_md5_varchar(const std::string& str) noexcept
{
    types::md5_varchar array{};
    // potentially truncate the string
    const std::size_t min_size{ std::min(array.size(), str.size()) };
    std::copy(str.cbegin(), std::next(str.cbegin(), min_size), array.begin());
    return array;
}
