#include "crypto.h"

#include <openssl/evp.h>

#include <algorithm>
#include <array>
#include <format>
#include <memory>
#include <random>

std::string crypto::sha512(const std::string& str)
{
    const auto ctx{ std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>(EVP_MD_CTX_new(), EVP_MD_CTX_free) };
    if (EVP_DigestInit_ex(ctx.get(), EVP_sha512_256(), nullptr) != 1)
        return {};
    if (EVP_DigestUpdate(ctx.get(), str.data(), str.size()) != 1)
        return {};

    std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
    unsigned int digest_len_raw{};
    if (EVP_DigestFinal_ex(ctx.get(), digest.data(), &digest_len_raw) != 1)
        return {};

    const std::size_t digest_len{ digest_len_raw };
    std::string result;
    result.resize_and_overwrite(digest_len * 2, [&digest, &digest_len](char* buffer, std::size_t) {
        for (std::size_t i{ 0 }; i < digest_len; ++i) {
            std::format_to(std::next(buffer, static_cast<std::ptrdiff_t>(i * 2)), "{:02x}", digest.at(i));
        }
        return digest_len * 2;
    });

    return result;
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
