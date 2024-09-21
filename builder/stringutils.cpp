#include "stringutils.h"

#include <algorithm>
#include <array>
#include <memory>
#include <ranges>

#include <openssl/evp.h>

std::string su::join(const std::vector<std::string>& list, char delim) noexcept
{
    const std::string str{ std::ranges::fold_left(list | std::views::join_with(delim), std::string{}, std::plus{}) };
    return str;
}

std::vector<std::string> su::split(const std::string& str, char delim) noexcept
{
    // const std::vector<std::string> list{ std::ranges::split_view(str, delim) | std::ranges::to<std::vector>() };
    std::vector<std::string> list;
    for (auto&& item : std::views::split(str, delim)) {
        list.emplace_back(std::string(item.cbegin(), item.cend()));
    }
    return list;
}

// https://github.com/openssl/openssl/blob/master/demos/digest/EVP_MD_demo.c
std::string su::sha512(const std::string& str) noexcept
{
    const std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> md_context{ EVP_MD_CTX_new(), &EVP_MD_CTX_free };
    if (!EVP_DigestInit_ex(md_context.get(), EVP_sha3_512(), NULL))
        return {};

    if (!EVP_DigestUpdate(md_context.get(), str.c_str(), str.length()))
        return {};

    std::array<unsigned char, EVP_MAX_MD_SIZE> out;
    unsigned int md_length;
    if (!EVP_DigestFinal_ex(md_context.get(), out.data(), &md_length))
        return {};

    std::string res;
    res.reserve(md_length * 2);

    constexpr const char* hex_chars{ "0123456789abcdef" };
    for (unsigned int i{ 0 }; i < md_length; ++i) {
        res.push_back(hex_chars[(out[i] >> 4) & 0xF]); // higher nibble
        res.push_back(hex_chars[out[i] & 0xF]);        // lower nibble
    }

    return res;
}
