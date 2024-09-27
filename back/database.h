#pragma once

#include <string>
#include <vector>

namespace database
{
    bool create_tables() noexcept;
    bool is_open() noexcept;

    std::vector<std::string> most_viewed() noexcept;

    std::string video_title(const std::string& video_id) noexcept;
    int video_views(const std::string& video_id) noexcept;
    std::string video_uploader(const std::string& video_id) noexcept;

    bool is_admin(const std::string& username) noexcept;
    bool is_valid_username(const std::string& username) noexcept;
    void add_user(const std::string& username, const std::string& password) noexcept;
    void add_admin(const std::string& username, const std::string& password) noexcept;
    std::string get_password(const std::string& username) noexcept;

    int user_count() noexcept;
    int video_count() noexcept;
    int view_count() noexcept;

    std::vector<std::string> user_list() noexcept;
}
