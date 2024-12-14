#pragma once

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

class Database
{
public:
    Database(const std::filesystem::path& path, bool& create_ok) noexcept;

    [[nodiscard]] bool create_tables() const noexcept;

    std::vector<std::int64_t> video_list() const noexcept;
    std::vector<std::int64_t> video_list(const std::int64_t& user_id) const noexcept;
    std::vector<std::int64_t> no_right_video_list() const noexcept;

    std::string video_title(const std::int64_t& id) const noexcept;
    std::int64_t video_views(const std::int64_t& id) const noexcept;
    bool video(const std::int64_t& id, const std::function<bool(const char*, std::size_t)>& callback) const noexcept;
    std::int64_t video_size(const std::int64_t& id) const noexcept;
    std::string thumbnail(const std::int64_t& id) const noexcept;

    [[nodiscard]] std::optional<std::int64_t> add_super_admin(const std::string& name, const std::string& password, const std::string& salt) const noexcept;
    bool is_super_admin(const std::int64_t& id) const noexcept;

    bool is_admin(const std::int64_t& id) const noexcept;
    bool is_user(const std::int64_t& id) const noexcept;
    [[nodiscard]] std::optional<std::int64_t> add_admin(const std::string& name, const std::string& password, const std::string& salt) const noexcept;
    [[nodiscard]] std::optional<std::int64_t> add_user(const std::string& name, const std::string& password, const std::string& salt) const noexcept;
    [[nodiscard]] bool update_user(const std::int64_t& id, const std::string& password) const noexcept;
    [[nodiscard]] bool delete_user(const std::int64_t& id) const noexcept;

    std::int64_t user_id(const std::string& name) const noexcept;
    std::string user_name(const std::int64_t& id) const noexcept;
    std::string user_password(const std::int64_t& id) const noexcept;
    std::string user_salt(const std::int64_t& id) const noexcept;

    std::int64_t user_count() const noexcept;
    std::int64_t video_count() const noexcept;
    std::int64_t view_count() const noexcept;

    std::vector<std::int64_t> user_list() const noexcept;
    std::vector<std::int64_t> admin_list() const noexcept;

    [[nodiscard]] std::optional<std::int64_t> add_video(const std::string& title, const std::string& video) const noexcept;
    [[nodiscard]] std::optional<std::int64_t> add_video_size(const std::int64_t& id, const std::int64_t& size) const noexcept;
    [[nodiscard]] std::optional<std::int64_t> add_video_thumbnail(const std::int64_t& id, const std::string& thumbnail) const noexcept;
    [[nodiscard]] bool add_video_rights(const std::int64_t& id, const std::vector<std::int64_t>& user_ids) const noexcept;
    [[nodiscard]] std::optional<std::int64_t> update_video_title(const std::int64_t& id, const std::string& title) const noexcept;
    [[nodiscard]] bool update_video_rights(const std::int64_t& id, const std::vector<std::int64_t>& user_ids) const noexcept;
    [[nodiscard]] bool delete_video(const std::int64_t& id) const noexcept;
    [[nodiscard]] bool increment_video_views(const std::int64_t& id) const noexcept;
    bool has_video_right(const std::int64_t& id) const noexcept;
    bool has_video_right(const std::int64_t& id, const std::int64_t& user_id) const noexcept;

    std::vector<std::int64_t> video_right_list(const std::int64_t& id) const noexcept;

private:
    const std::filesystem::path path_;
};
