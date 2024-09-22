#pragma once

#include <string>
#include <vector>

namespace database
{
    std::vector<std::string> most_viewed() noexcept;

    bool is_valid_username(const std::string& username) noexcept;
    void add_user(const std::string& username, const std::string& password) noexcept;
    void add_admin(const std::string& username, const std::string& password) noexcept;
    std::string get_password(const std::string& username) noexcept;
    void update_session(const std::string& session_id, const std::string& username) noexcept;
}
