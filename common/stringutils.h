#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

namespace su
{
    std::vector<std::string> split(const std::string& str, char delim = ' ');

    std::string trim(const std::string& str);
    std::string lower(const std::string& str);

    bool string_to_bool(const std::string& str);
    int string_to_int(const std::string& str);
    std::string int_to_string(int val);

    std::string time_point_to_string(std::chrono::system_clock::time_point tp);
    std::optional<std::chrono::system_clock::time_point> string_to_time_point(const std::string& str);

    std::string seconds_to_string(std::chrono::seconds d);
    std::chrono::seconds string_to_seconds(const std::string& str);
}
