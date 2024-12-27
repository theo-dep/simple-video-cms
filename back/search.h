#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace search
{
    std::vector<int> extract(const std::string& search,
                             const std::unordered_map<std::string, int>& choices, /* title/id */
                             double score_cutoff = 0.0) noexcept;
}
