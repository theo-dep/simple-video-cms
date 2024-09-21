#pragma once

#include <string>
#include <vector>

namespace su
{
    std::string join(const std::vector<std::string>& list, char delim = ' ') noexcept;
    std::vector<std::string> split(const std::string& str, char delim = ' ') noexcept;

    std::string sha512(const std::string& str) noexcept;
}
