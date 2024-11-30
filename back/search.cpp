#include "search.h"

#include "logging.h"
#include "stringutils.h"

#include <rapidfuzz.hpp>

#include <execution>
#include <map>

// github.com/rapidfuzz/rapidfuzz-cpp/tree/main?tab=readme-ov-file#multithreading
std::vector<std::int64_t> search::extract(const std::string& search,
                                          const std::unordered_map<std::string, std::int64_t>& choices,
                                          double score_cutoff) noexcept
{
    std::string processed_search{ search };
    su::lower(processed_search);
    su::trim(processed_search);

    const rapidfuzz::fuzz::CachedPartialTokenSetRatio scorer_set(processed_search);
    const rapidfuzz::fuzz::CachedPartialTokenSortRatio scorer_sort(processed_search);

    std::multimap<double, std::string, std::greater<>> scored_result_map;

    using choice_type = std::remove_cvref_t<decltype(choices)>::value_type;
    using score_type = decltype(scored_result_map)::value_type;

    std::for_each(std::execution::par, choices.cbegin(), choices.cend(),
                  [&](const choice_type& pair) noexcept {
                      const std::string& str{ pair.first };
                      std::string processed_str{ str };
                      su::lower(processed_str);
                      su::trim(processed_str);

                      const double score_set{ scorer_set.similarity(processed_str, score_cutoff) };
                      const double score_sort{ scorer_sort.similarity(processed_str, score_cutoff) };
                      logging::debug{
                          "search: {}, sentence: {}, score set: {}, score sort: {}",
                          processed_search, str, score_set, score_sort
                      };

                      const double score{ score_set + score_sort };
                      if (score >= score_cutoff) {
                          scored_result_map.emplace(score, str);
                      }
                  });

    std::vector<std::int64_t> results(scored_result_map.size());
    std::ranges::transform(scored_result_map, results.begin(),
                           [&choices](const score_type& pair) noexcept -> std::int64_t {
                               return choices.at(pair.second);
                           });
    return results;
}
