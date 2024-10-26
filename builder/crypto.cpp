#include "crypto.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wreorder"
#include <cstring>
#include <hashpp.h>
#pragma GCC diagnostic pop

#include <algorithm>
#include <cassert>

std::string crypto::sha512(const std::string& str) noexcept
{
    const hashpp::hash hash{ hashpp::get::getHash(hashpp::ALGORITHMS::SHA2_512_256, str) };
    return hash.getString();
}

types::md5_varchar crypto::md5(const std::string& str) noexcept
{
    const hashpp::hash hash{ hashpp::get::getHash(hashpp::ALGORITHMS::MD5, str) };
    const std::string& hash_str{ hash.getString() };

    types::md5_varchar res{};
    assert(hash_str.size() == res.size());
    std::ranges::copy(hash_str, res.begin());
    return res;
}
