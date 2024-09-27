#include "crypto.h"

#include <openssl/evp.h>

#include <array>
#include <memory>

// https://github.com/openssl/openssl/blob/master/demos/digest/EVP_MD_demo.c
std::string crypto::sha512(const std::string& str) noexcept
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
