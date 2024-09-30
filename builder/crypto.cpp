#include "crypto.h"

#include <openssl/evp.h>

#include <array>
#include <memory>

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
        res.reserve(static_cast<unsigned long>(md_length) * 2);

        constexpr std::array<char, 16> hex_chars{ '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
        for (decltype(out)::const_iterator it{ out.cbegin() }; it != std::next(out.cbegin(), md_length) && it != out.cend(); ++it) {
            constexpr unsigned char max_hex{ 0xF };
            res.push_back(hex_chars.at((*it >> 4) & max_hex)); // higher nibble
            res.push_back(hex_chars.at(*it & max_hex));        // lower nibble
        }

        return res;
    } catch (...) {
        return {};
    }
}
