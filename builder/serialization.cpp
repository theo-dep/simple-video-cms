#include "serialization.h"

#include <algorithm>
#include <ranges>

std::string sz::join(const std::vector<std::string>& list, char delim)
{
    const std::string str{ std::ranges::fold_left(list | std::views::join_with(delim), std::string{}, std::plus{}) };
    return str;
}

std::vector<std::string> sz::split(const std::string& str, char delim)
{
    // const std::vector<std::string> list{ std::ranges::split_view(str, delim) | std::ranges::to<std::vector>() };
    std::vector<std::string> list;
    for (auto&& item : std::views::split(str, delim)) {
        list.emplace_back(std::string(item.cbegin(), item.cend()));
    }
    return list;
}
