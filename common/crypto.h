#pragma once

#include <string>

namespace crypto
{
    std::string sha512(const std::string& str);

    static constexpr auto default_string_length = 16;
    std::string random_string(std::string::size_type length = default_string_length);
    std::string password(const std::string& raw_password, const std::string& salt);
}
