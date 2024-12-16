#pragma once

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

class Database
{
public:
    Database(const std::filesystem::path& path) noexcept;

    [[nodiscard]] bool create_tables() const noexcept;

    std::vector<int> admin_video_list() const noexcept;
    std::vector<int> user_video_list(int user_id) const noexcept;
    std::vector<int> no_user_video_list() const noexcept;

    std::string video_title(int id) const noexcept;
    int video_views(int id) const noexcept;
    int video_size(int id) const noexcept;
    bool video(int id, const std::function<bool(const char*, std::size_t)>& callback) const noexcept;
    std::string thumbnail(int id) const noexcept;

    [[nodiscard]] std::optional<int> add_super_admin(const std::string& name, const std::string& password, const std::string& salt) const noexcept;
    bool is_super_admin(int id) const noexcept;

    bool is_admin(int id) const noexcept;
    bool is_user(int id) const noexcept;
    [[nodiscard]] std::optional<int> add_admin(const std::string& name, const std::string& password, const std::string& salt) const noexcept;
    [[nodiscard]] std::optional<int> add_user(const std::string& name, const std::string& password, const std::string& salt) const noexcept;
    [[nodiscard]] std::optional<int> update_user_name(int id, const std::string& name) const noexcept;
    [[nodiscard]] std::optional<int> update_user_password(int id, const std::string& password) const noexcept;
    [[nodiscard]] bool delete_user(int id) const noexcept;

    int user_id(const std::string& name) const noexcept;
    std::string user_name(int id) const noexcept;
    std::string user_password(int id) const noexcept;
    std::string user_salt(int id) const noexcept;

    int user_count() const noexcept;
    int video_count() const noexcept;
    int view_count() const noexcept;

    std::vector<int> user_list() const noexcept;
    std::vector<int> admin_list() const noexcept;

    [[nodiscard]] std::optional<int> add_video(const std::string& title, const std::string& video_content) const noexcept;
    [[nodiscard]] std::optional<int> add_video_thumbnail(int id, const std::string& thumbnail_content) const noexcept;
    [[nodiscard]] bool add_video_rights(int id, const std::vector<int>& user_ids) const noexcept;
    [[nodiscard]] std::optional<int> update_video_title(int id, const std::string& title) const noexcept;
    [[nodiscard]] bool update_video_rights(int id, const std::vector<int>& user_ids) const noexcept;
    [[nodiscard]] bool delete_video(int id) const noexcept;
    [[nodiscard]] bool increment_video_views(int id) const noexcept;
    bool has_video_right(int id) const noexcept;
    bool has_video_right(int id, int user_id) const noexcept;

    std::vector<int> video_right_list(int id) const noexcept;

private:
    const std::filesystem::path path_;
};
