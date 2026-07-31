#pragma once

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
}
