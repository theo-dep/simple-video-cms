#pragma once

#include <string>

namespace video
{
    static constexpr auto default_video_format = "mp4";
    static constexpr auto default_thumbnail_width = 320;
    static constexpr auto default_thumbnail_height = 180;
    static constexpr auto default_thumbnail_n_images = 100;
    static constexpr auto default_hls_time = 6;

    std::string thumbnail(const std::string& video_content, const std::string& format = default_video_format,
                          std::size_t width = default_thumbnail_width, std::size_t height = default_thumbnail_height, std::size_t n_images = default_thumbnail_n_images);

    bool convert_to_hls(const std::string& video_content, const std::string& out_dir, const std::string& name,
                        const std::string& format = default_video_format, int hls_time = default_hls_time);
}