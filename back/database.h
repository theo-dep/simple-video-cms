#pragma once

#include "types.h"

#include <filesystem>
#include <string>
#include <vector>

class Database
{
public:
    Database(const std::filesystem::path& path, bool& create_ok) noexcept;

    bool create_tables() const noexcept;

    std::vector<std::int64_t> video_list() const noexcept;
    std::vector<std::int64_t> video_list(const std::string& username) const noexcept;
    std::vector<std::int64_t> no_right_video_list() const noexcept;

    std::string video_title(const std::int64_t& id) const noexcept;
    int video_views(const std::int64_t& id) const noexcept;
    std::string video_file_path(const std::int64_t& id) const noexcept;

    void add_super_admin(const std::string& username, const std::string& password, const std::string& salt) const noexcept;
    bool is_super_admin(const std::string& username) const noexcept;

    bool is_admin(const std::string& username) const noexcept;
    bool is_user(const std::string& username) const noexcept;
    void add_admin(const std::string& username, const std::string& password, const std::string& salt) const noexcept;
    void add_user(const std::string& username, const std::string& password, const std::string& salt) const noexcept;
    void update_admin(const std::string& username, const std::string& password) const noexcept;
    void update_user(const std::string& username, const std::string& password) const noexcept;
    void delete_admin(const std::string& username) const noexcept;
    void delete_user(const std::string& username) const noexcept;
    std::string user_password(const std::string& username) const noexcept;
    std::string user_salt(const std::string& username) const noexcept;

    int user_count() const noexcept;
    int video_count() const noexcept;
    int view_count() const noexcept;

    std::vector<std::string> user_list() const noexcept;
    std::vector<std::string> admin_list() const noexcept;

    void add_video(const std::int64_t& id, const std::string& title, const std::string& file_path) const noexcept;
    void add_video_rights(const std::int64_t& id, const std::vector<std::string>& usernames) const noexcept;
    void update_video_rights(const std::int64_t& id, const std::vector<std::string>& usernames) const noexcept;
    void delete_video(const std::int64_t& id) const noexcept;
    void increment_video_views(const std::int64_t& id) const noexcept;
    bool has_video_right(const std::int64_t& id) const noexcept;
    bool has_video_right(const std::int64_t& id, const std::string& username) const noexcept;

    std::vector<std::string> video_right_list(const std::int64_t& id) const noexcept;

private:
    std::filesystem::path path_;
};
