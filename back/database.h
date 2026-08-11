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

    bool bookmarked(int user_id, int video_id) const;
    bool set_bookmark(int user_id, int video_id, bool bookmarked) const;

    std::optional<Video> video(int video_id) const;
    int video_size(int video_id) const;
    std::string video(int video_id, std::size_t offset, std::size_t length) const;
    std::string video_playlist(int video_id) const;
    std::string video_segment(int video_id, const std::string& segment) const;
    std::string thumbnail(int video_id) const;

    [[nodiscard]] std::optional<int> add_super_admin(const std::string& name, const std::string& salt) const;
    bool is_super_admin(int user_id) const;

    bool is_admin(int user_id) const;
    [[nodiscard]] std::optional<int> add_admin(const std::string& name, const std::string& salt) const;
    [[nodiscard]] std::optional<int> add_user(const std::string& name, const std::string& salt) const;
    [[nodiscard]] std::optional<int> add_password(int user_id, const std::string& password) const;
    [[nodiscard]] std::optional<int> update_username(int user_id, const std::string& name) const;
    [[nodiscard]] std::optional<int> update_password(int user_id, const std::string& password) const;
    [[nodiscard]] std::optional<int> clear_password(int user_id) const;
    [[nodiscard]] std::optional<int> deactivate_user(int user_id, bool deactivated) const;
    [[nodiscard]] bool delete_user(int user_id) const;

    bool user_exists(const std::string& name) const;
    bool user_exists(int user_id, const std::string& name) const;
    std::optional<User> user(const std::string& name) const;
    std::optional<User> user(int user_id) const;
    bool deactivated_user(int user_id) const;

    int user_count() const;
    int group_count() const;
    int video_count() const;

    std::vector<User> user_list() const;
    std::vector<User> admin_list() const;

    std::vector<Group> group_list() const;

    bool group_exists(const std::string& name) const;
    bool group_exists(int group_id, const std::string& name) const;
    std::optional<Group> group(int group_id) const;
    [[nodiscard]] std::optional<int> add_group(const std::string& name) const;
    [[nodiscard]] bool add_group_users(int group_id, const std::vector<int>& user_ids) const;
    [[nodiscard]] bool add_group_video_rights(int group_id, const std::vector<int>& video_ids) const;
    [[nodiscard]] bool add_user_groups(int user_id, const std::vector<int>& group_ids) const;
    [[nodiscard]] bool add_user_video_rights(int user_id, const std::vector<int>& video_ids) const;
    [[nodiscard]] std::optional<int> update_group_name(int group_id, const std::string& name) const;
    [[nodiscard]] bool update_group_users(int group_id, const std::vector<int>& user_ids) const;
    [[nodiscard]] bool update_group_video_rights(int group_id, const std::vector<int>& video_ids) const;
    [[nodiscard]] bool update_user_groups(int user_id, const std::vector<int>& group_ids) const;
    [[nodiscard]] bool update_user_video_rights(int user_id, const std::vector<int>& video_ids) const;
    [[nodiscard]] bool delete_group(int group_id) const;

    std::vector<User> group_user_list(int group_id) const;
    std::vector<Video> group_video_list(int group_id) const;
    std::vector<Group> user_group_list(int user_id) const;
    std::vector<Video> unique_user_video_list(int user_id) const;

    static std::string hls_video_name(int video_id);
    std::filesystem::path hls_video_path(int video_id) const;

    [[nodiscard]] std::optional<int> add_video(const std::string& title, const std::optional<std::string>& date, const std::optional<int>& place_id, const std::string& video_content) const;
    [[nodiscard]] std::optional<int> add_video_thumbnail(int video_id, const std::string& thumbnail_content) const;
    [[nodiscard]] bool add_video_authors(int video_id, const std::vector<int>& author_ids) const;
    [[nodiscard]] bool add_video_tags(int video_id, const std::vector<int>& tag_ids) const;
    [[nodiscard]] bool add_video_group_rights(int video_id, const std::vector<int>& group_ids) const;
    [[nodiscard]] bool add_video_user_rights(int video_id, const std::vector<int>& user_ids) const;
    [[nodiscard]] std::optional<int> update_video(int video_id, const std::string& title, const std::optional<std::string>& date, const std::optional<int>& place_id) const;
    [[nodiscard]] bool update_video_authors(int video_id, const std::vector<int>& author_ids) const;
    [[nodiscard]] bool update_video_tags(int video_id, const std::vector<int>& tag_ids) const;
    [[nodiscard]] bool update_video_group_rights(int video_id, const std::vector<int>& group_ids) const;
    [[nodiscard]] bool update_video_user_rights(int video_id, const std::vector<int>& user_ids) const;
    [[nodiscard]] bool delete_video(int video_id) const;
    bool has_video_right(int video_id) const;
    bool has_video_right(int video_id, int user_id) const;

    std::vector<Place> place_list() const;
    std::optional<Place> place(int place_id) const;
    bool place_exists(const std::string& name) const;
    [[nodiscard]] std::optional<int> add_place(const std::string& name) const;
    [[nodiscard]] bool delete_place(int place_id) const;

    std::vector<Author> author_list() const;
    std::optional<Author> author(int author_id) const;
    bool author_exists(const std::string& name) const;
    [[nodiscard]] std::optional<int> add_author(const std::string& name) const;
    [[nodiscard]] bool delete_author(int author_id) const;

    std::vector<Tag> tag_list() const;
    std::optional<Tag> tag(int tag_id) const;
    bool tag_exists(const std::string& name) const;
    [[nodiscard]] std::optional<int> add_tag(const std::string& name) const;
    [[nodiscard]] bool delete_tag(int tag_id) const;

    std::vector<Author> video_author_list(int video_id) const;
    std::vector<Tag> video_tag_list(int video_id) const;

    std::vector<Group> video_group_right_list(int video_id) const;
    std::vector<User> video_user_right_list(int video_id) const;

    std::vector<std::tuple<std::string, int, std::string, std::string>> session_list() const;
    std::optional<SessionInfo> session(const std::string& session_id) const;
    void add_session(const std::string& session_id, int user_id, const std::string& creation_date, const std::string& max_age_time) const;
    [[nodiscard]] bool delete_session(const std::string& session_id) const;

protected:
    std::filesystem::path base_path() const;
    std::filesystem::path video_path() const;
    std::filesystem::path video_path(int video_id) const;
    std::filesystem::path thumbnail_path() const;
    std::filesystem::path thumbnail_path(int video_id) const;

private:
    const std::filesystem::path _path;
    mutable std::shared_mutex _mutex;
};
