#pragma once

#include <string>
#include <vector>

namespace su
{
    std::string join(const std::vector<std::string>& list, char delim = ' ');
    std::vector<std::string> split(const std::string& str, char delim = ' ');

    void trim(std::string& str);
    void lower(std::string& str);

    std::string bool_to_string(bool b);
    bool string_to_bool(const std::string& str);

    int string_to_int(const std::string& str);
    std::string int_to_string(int val);
}
