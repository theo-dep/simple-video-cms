#pragma once

#include <string>

namespace crypto
{
    std::string sha512(const std::string& str) noexcept;

    std::string random_string(std::string::size_type length = 16) noexcept;
    std::string password(const std::string& raw_password, const std::string& salt) noexcept;
}
