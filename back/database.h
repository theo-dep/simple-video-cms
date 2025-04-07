#pragma once

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

class Database
{
public:
    Database(std::filesystem::path path);

    [[nodiscard]] bool create_tables() const;

    std::vector<int> admin_video_list() const;
    std::vector<int> user_video_list(int user_id) const;
    std::vector<int> no_user_video_list() const;

    std::string video_title(int id) const;
    int video_views(int id) const;
    int video_size(int id) const;
    std::string video(int id, std::size_t offset, std::size_t length) const;
    std::string thumbnail(int id) const;

    [[nodiscard]] std::optional<int> add_super_admin(const std::string& name, const std::string& password, const std::string& salt) const;
    bool is_super_admin(int id) const;

    bool is_admin(int id) const;
    bool is_user(int id) const;
    [[nodiscard]] std::optional<int> add_admin(const std::string& name, const std::string& salt) const;
    [[nodiscard]] std::optional<int> add_user(const std::string& name, const std::string& salt) const;
    [[nodiscard]] std::optional<int> update_user_name(int id, const std::string& name) const;
    [[nodiscard]] std::optional<int> update_user_password(int id, const std::string& password) const;
    [[nodiscard]] bool delete_user(int id) const;

    int user_id(const std::string& name) const;
    std::string user_name(int id) const;
    std::optional<std::string> user_password(int id) const;
    std::string user_salt(int id) const;

    int user_count() const;
    int video_count() const;
    int view_count() const;

    std::vector<int> user_list() const;
    std::vector<int> admin_list() const;

    [[nodiscard]] std::optional<int> add_video(const std::string& title, const std::string& video_content) const;
    [[nodiscard]] std::optional<int> add_video_thumbnail(int id, const std::string& thumbnail_content) const;
    [[nodiscard]] bool add_video_rights(int id, const std::vector<int>& user_ids) const;
    [[nodiscard]] std::optional<int> update_video_title(int id, const std::string& title) const;
    [[nodiscard]] bool update_video_rights(int id, const std::vector<int>& user_ids) const;
    [[nodiscard]] bool delete_video(int id) const;
    [[nodiscard]] bool increment_video_views(int id) const;
    bool has_video_right(int id) const;
    bool has_video_right(int id, int user_id) const;

    std::vector<int> video_right_list(int id) const;

protected:
    std::filesystem::path base_path() const;
    std::filesystem::path video_path() const;
    std::filesystem::path video_path(int id) const;
    std::filesystem::path thumbnail_path() const;
    std::filesystem::path thumbnail_path(int id) const;

private:
    const std::filesystem::path _path;
};
