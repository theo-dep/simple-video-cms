#pragma once

#include <string>
#include <vector>

namespace client
{
    std::string get_404_error() noexcept;
    std::string get_403_error() noexcept;
    std::string get_generic_error(int error, const std::string& message) noexcept;

    std::string get_homepage() noexcept;

    std::string get_login() noexcept;
    std::string get_signup() noexcept;

    std::vector<std::string> get_most_viewed() noexcept;

    std::string video_title(const std::string& id) noexcept;
    int video_views(const std::string& id) noexcept;
    std::string video_uploader(const std::string& id) noexcept;

    bool is_admin(const std::string& session_id) noexcept;
    bool is_valid_username(const std::string& username) noexcept;
    void add_user(const std::string& username, const std::string& password) noexcept;
    bool is_valid_user(const std::string& username, const std::string& password) noexcept;
    void update_session(const std::string& session_id, const std::string& username) noexcept;
}
