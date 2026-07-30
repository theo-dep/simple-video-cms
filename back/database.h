#pragma once

#include "databasemodel.h"

#include <filesystem>
#include <functional>
#include <optional>
#include <shared_mutex>
#include <string>
#include <vector>

class Database
{
public:
    Database(std::filesystem::path path);

    [[nodiscard]] bool create_tables() const;

    std::vector<Video> admin_video_list() const;
    std::vector<Video> user_video_list(int user_id) const;
    std::vector<Video> no_user_video_list() const;

    bool video_exists(const std::string& title) const;
    bool video_exists(int id, const std::string& title) const;
    std::string video_title(int id) const;
    int video_size(int id) const;
    std::string video(int id, std::size_t offset, std::size_t length) const;
    std::string video_playlist(int id) const;
    std::string video_segment(int id, const std::string& segment) const;
    std::string thumbnail(int id) const;

    [[nodiscard]] std::optional<int> add_super_admin(const std::string& name, const std::string& salt) const;
    bool is_super_admin(int id) const;

    bool is_admin(int id) const;
    [[nodiscard]] std::optional<int> add_admin(const std::string& name, const std::string& salt) const;
    [[nodiscard]] std::optional<int> add_user(const std::string& name, const std::string& salt) const;
    [[nodiscard]] std::optional<int> add_password(int id, const std::string& password) const;
    [[nodiscard]] std::optional<int> update_username(int id, const std::string& name) const;
    [[nodiscard]] std::optional<int> update_password(int id, const std::string& password) const;
    [[nodiscard]] std::optional<int> clear_password(int id) const;
    [[nodiscard]] bool delete_user(int id) const;

    int user_id(const std::string& name) const;
    bool user_exists(const std::string& name) const;
    bool user_exists(int id, const std::string& name) const;
    std::string user_name(int id) const;
    std::optional<std::string> user_password(int id) const;
    std::string user_salt(int id) const;

    int user_count() const;
    int group_count() const;
    int video_count() const;

    std::vector<User> user_list() const;
    std::vector<User> admin_list() const;

    std::vector<Group> group_list() const;

    bool group_exists(const std::string& name) const;
    bool group_exists(int id, const std::string& name) const;
    std::string group_name(int id) const;
    [[nodiscard]] std::optional<int> add_group(const std::string& name) const;
    [[nodiscard]] bool add_group_users(int id, const std::vector<int>& user_ids) const;
    [[nodiscard]] bool add_group_video_rights(int id, const std::vector<int>& video_ids) const;
    [[nodiscard]] bool add_user_groups(int user_id, const std::vector<int>& group_ids) const;
    [[nodiscard]] std::optional<int> update_group_name(int id, const std::string& name) const;
    [[nodiscard]] bool update_group_users(int id, const std::vector<int>& user_ids) const;
    [[nodiscard]] bool update_group_video_rights(int id, const std::vector<int>& video_ids) const;
    [[nodiscard]] bool update_user_groups(int user_id, const std::vector<int>& group_ids) const;
    [[nodiscard]] bool delete_group(int id) const;

    std::vector<User> group_user_list(int group_id) const;
    std::vector<Video> group_video_list(int group_id) const;
    std::vector<Group> user_group_list(int user_id) const;

    [[nodiscard]] std::optional<int> add_video(const std::string& title, const std::string& video_content) const;
    static std::string hls_video_name(int id);
    std::filesystem::path hls_video_path(int id) const;

    [[nodiscard]] std::optional<int> add_video_thumbnail(int id, const std::string& thumbnail_content) const;
    [[nodiscard]] bool add_video_group_rights(int id, const std::vector<int>& group_ids) const;
    [[nodiscard]] bool add_video_user_rights(int id, const std::vector<int>& user_ids) const;
    [[nodiscard]] std::optional<int> update_video_title(int id, const std::string& title) const;
    [[nodiscard]] bool update_video_group_rights(int id, const std::vector<int>& group_ids) const;
    [[nodiscard]] bool update_video_user_rights(int id, const std::vector<int>& user_ids) const;
    [[nodiscard]] bool delete_video(int id) const;
    bool has_video_right(int id) const;
    bool has_video_right(int id, int user_id) const;

    std::vector<Group> video_group_right_list(int id) const;
    std::vector<User> video_user_right_list(int id) const;

protected:
    std::filesystem::path base_path() const;
    std::filesystem::path video_path() const;
    std::filesystem::path video_path(int id) const;
    std::filesystem::path thumbnail_path() const;
    std::filesystem::path thumbnail_path(int id) const;

private:
    const std::filesystem::path _path;
    mutable std::shared_mutex _mutex;
};
