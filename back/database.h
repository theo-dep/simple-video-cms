#pragma once

#include "types.h"

#include <string>
#include <vector>

namespace database
{
    bool create_tables() noexcept;
    bool is_open() noexcept;

    std::vector<types::md5_varchar> video_list() noexcept;
    std::vector<types::md5_varchar> most_viewed() noexcept;

    std::string video_title(const types::md5_varchar& id) noexcept;
    int video_views(const types::md5_varchar& id) noexcept;
    std::string video_file_path(const types::md5_varchar& id) noexcept;

    void add_super_admin(const std::string& username, const std::string& password) noexcept;
    bool is_super_admin(const std::string& username) noexcept;

    bool is_admin(const std::string& username) noexcept;
    bool is_user(const std::string& username) noexcept;
    void add_admin(const std::string& username, const std::string& password) noexcept;
    void add_user(const std::string& username, const std::string& password) noexcept;
    void update_admin(const std::string& username, const std::string& password) noexcept;
    void update_user(const std::string& username, const std::string& password) noexcept;
    void delete_admin(const std::string& username) noexcept;
    void delete_user(const std::string& username) noexcept;
    std::string get_password(const std::string& username) noexcept;

    int user_count() noexcept;
    int video_count() noexcept;
    int view_count() noexcept;

    std::vector<std::string> user_list() noexcept;
    std::vector<std::string> admin_list() noexcept;

    void add_video(const types::md5_varchar& id, const std::string& title, const std::string& file_path) noexcept;
    void add_video_rights(const types::md5_varchar& id, const std::vector<std::string>& usernames) noexcept;
    void delete_video(const types::md5_varchar& id) noexcept;
    void increment_video_views(const types::md5_varchar& id) noexcept;
}
