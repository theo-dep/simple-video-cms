#pragma once

#include <cstdint>
#include <string>

namespace video
{
    std::string extract_first_frame(const std::string& video_content, const std::string& format = "mp4", std::int64_t timestamp = 5) noexcept;
}