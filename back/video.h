#pragma once

#include <cstdint>
#include <string>

namespace video
{
    std::string thumbnail(const std::string& video_content, const std::string& format = "mp4",
                          std::size_t width = 320, std::size_t height = 180, std::size_t n_images = 100);

    bool convert_to_hls(const std::string& video_content, const std::string& out_dir, const std::string& name,
                        const std::string& format = "mp4", int hls_time = 6);
}