#pragma once

#include <array>

namespace types
{
    static constexpr std::size_t md5_length{ 128 / 8 };
    using md5_varchar = std::array<char, 2 * md5_length>;
}
