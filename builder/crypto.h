#pragma once

#include "types.h"

#include <string>

namespace crypto
{
    std::string sha512(const std::string& str) noexcept;

    types::md5_varchar sha1(const std::string& str) noexcept;
}
