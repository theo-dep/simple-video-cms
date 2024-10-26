#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace search
{
    std::vector<std::int64_t> extract(const std::string& search,
                                      const std::unordered_map<std::string, std::int64_t>& choices, /* title/id */
                                      double score_cutoff = 0.0) noexcept;
}
