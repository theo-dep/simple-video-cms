#include "crypto.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wreorder"
#include <cstring>
#include <hashpp.h>
#pragma GCC diagnostic pop

#include <algorithm>
#include <array>
#include <random>

std::string crypto::sha512(const std::string& str)
{
    const hashpp::hash hash{ hashpp::get::getHash(hashpp::ALGORITHMS::SHA2_512_256, str) };
    return hash.getString();
}

std::string crypto::random_string(std::string::size_type length)
{
    static constexpr std::array charset{
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
        'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
        'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'
    };
    thread_local static std::mt19937 rg{ std::random_device{}() };
    thread_local static std::uniform_int_distribution<std::string::size_type> pick(0, charset.size() - 1);

    std::string str(length, 0);
    std::generate_n(str.begin(), length, []() -> char { return charset.at(pick(rg)); });
    return str;
}

std::string crypto::password(const std::string& raw_password, const std::string& salt)
{
    return sha512(raw_password + salt);
}
