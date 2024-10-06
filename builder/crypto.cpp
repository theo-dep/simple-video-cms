#include "crypto.h"

#include <openssl/evp.h>

#include <array>
#include <cassert>
#include <memory>

namespace crypto
{
    template <typename Container, std::size_t Size = EVP_MAX_MD_SIZE>
        requires(std::is_same_v<typename Container::value_type, char>)
    constexpr void to_char_array(Container& container, const std::array<unsigned char, Size>& array, unsigned int md_length) noexcept;
}

template <typename Container, std::size_t Size>
    requires(std::is_same_v<typename Container::value_type, char>)
constexpr void crypto::to_char_array(Container& container, const std::array<unsigned char, Size>& array, unsigned int md_length) noexcept
{
    assert(container.size() / 2 == md_length);
    constexpr std::array hex_chars{ '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
    std::size_t i{ 0 };
    for (typename std::array<unsigned char, Size>::const_iterator it{ array.cbegin() };
         it != std::next(array.cbegin(), md_length) && it != array.cend(); ++it) {
        constexpr unsigned char max_hex{ 0xF };
        container[i++] = hex_chars.at((*it >> 4) & max_hex); // higher nibble
        container[i++] = hex_chars.at(*it & max_hex);        // lower nibble
    }
}

// https://github.com/openssl/openssl/blob/master/demos/digest/EVP_MD_demo.c
std::string crypto::sha512(const std::string& str) noexcept
{
    try {
        const std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> md_context{ EVP_MD_CTX_new(), &EVP_MD_CTX_free };
        if (EVP_DigestInit_ex(md_context.get(), EVP_sha3_512(), nullptr) == 0)
            return {};

        if (EVP_DigestUpdate(md_context.get(), str.c_str(), str.length()) == 0)
            return {};

        std::array<unsigned char, EVP_MAX_MD_SIZE> out{};
        unsigned int md_length{ 0 };
        if (EVP_DigestFinal_ex(md_context.get(), out.data(), &md_length) == 0)
            return {};

        std::string res;
        res.resize(static_cast<unsigned long>(md_length) * 2);
        to_char_array(res, out, md_length);

        return res;
    } catch (...) {
        return {};
    }
}

types::md5_varchar crypto::sha1(const std::string& str) noexcept
{
    try {
        const std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> md_context{ EVP_MD_CTX_new(), &EVP_MD_CTX_free };
        if (EVP_DigestInit_ex(md_context.get(), EVP_md5(), nullptr) == 0)
            return {};

        if (EVP_DigestUpdate(md_context.get(), str.c_str(), str.length()) == 0)
            return {};

        std::array<unsigned char, EVP_MAX_MD_SIZE> out{};
        unsigned int md_length{ 0 };
        if (!EVP_DigestFinal_ex(md_context.get(), out.data(), &md_length))
            return {};

        types::md5_varchar res{};
        to_char_array(res, out, md_length);

        return res;
    } catch (...) {
        return {};
    }
}
