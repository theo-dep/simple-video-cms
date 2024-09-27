#pragma once

#include <string>
#include <vector>

namespace su
{
    std::string join(const std::vector<std::string>& list, char delim = ' ') noexcept;
    std::vector<std::string> split(const std::string& str, char delim = ' ') noexcept;

    void trim(std::string& str) noexcept;
    void lower(std::string& str) noexcept;

    std::string bool_to_string(bool b) noexcept;
    bool string_to_bool(const std::string& str) noexcept;
}
