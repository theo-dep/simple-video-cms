#pragma once

#include <string>
#include <vector>

namespace client
{
    std::string error_page_404() noexcept;
    std::string error_page_403() noexcept;
    std::string generic_error(int error, const std::string& message) noexcept;

    std::string home_page() noexcept;

    std::string login_page() noexcept;

    std::vector<std::string> most_viewed_video_list() noexcept;

    std::string video_title(const std::string& id) noexcept;
    int video_views(const std::string& id) noexcept;
    std::string video_uploader(const std::string& id) noexcept;

    bool is_admin(const std::string& username) noexcept;
    bool is_valid_username(const std::string& username) noexcept;
    void add_user(const std::string& username, const std::string& password) noexcept;
    bool is_valid_user(const std::string& username, const std::string& password) noexcept;

    int user_count() noexcept;
    int video_count() noexcept;
    int view_count() noexcept;
    std::string dashboard_page() noexcept;
}
