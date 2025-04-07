#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace httplib
{
    class Client;
}

class Client
{
public:
    Client();
    ~Client() = default;

    std::string error_page_404() const;
    std::string error_page_403() const;
    static std::string generic_error(int error, const std::string& message);

    std::pair<std::string, std::string> static_file(const std::string& file) const;

    std::string home_page() const;
    std::string dashboard_page() const;

    std::string login_page() const;

    std::string confirm_action_page() const;

    std::string user_list_page() const;
    std::string add_user_page() const;
    std::string update_user_page() const;

    std::string admin_list_page() const;

    std::string video_list_page() const;
    std::string add_video_page() const;
    std::string update_video_page() const;
    std::string watch_video_page() const;

    std::vector<std::string> admin_video_list() const;
    std::vector<std::string> admin_video_list(const std::string& search) const;
    std::vector<std::string> user_video_list(const std::string& user_id) const;
    std::vector<std::string> user_video_list(const std::string& user_id, const std::string& search) const;
    std::vector<std::string> no_user_video_list() const;
    std::vector<std::string> no_user_video_list(const std::string& search) const;

    std::string video_title(const std::string& video_id) const;
    int video_views(const std::string& video_id) const;

    bool is_admin(const std::string& user_id) const;
    bool is_super_admin(const std::string& user_id) const;
    bool is_user(const std::string& user_id) const;
    std::string user_name(const std::string& user_id) const;
    std::string user_id(const std::string& username) const;
    std::string add_admin(const std::string& username) const;
    std::string add_user(const std::string& username) const;
    void update_user_name(const std::string& user_id, const std::string& username) const;
    void update_user_password(const std::string& user_id, const std::string& password) const;
    void delete_user(const std::string& user_id) const;
    bool is_valid_user(const std::string& user_id, const std::string& password) const;

    int user_count() const;
    int video_count() const;
    int view_count() const;

    std::vector<std::string> user_list() const;
    std::vector<std::string> admin_list() const;

    std::string add_video(const std::string& title, const std::string& content, const std::vector<std::string>& allowed_user_ids) const;
    void update_video(const std::string& video_id, const std::string& title, const std::vector<std::string>& allowed_user_ids) const;
    void delete_video(const std::string& video_id) const;
    void increment_video_views(const std::string& video_id) const;
    std::string video(const std::string& video_id, std::size_t offset, std::size_t length) const;
    int video_size(const std::string& video_id) const;
    std::string thumbnail(const std::string& video_id) const;
    bool has_video_right(const std::string& video_id) const;
    bool has_video_right(const std::string& video_id, const std::string& user_id) const;

    std::vector<std::string> video_right_list(const std::string& video_id) const;

private:
    std::unique_ptr<httplib::Client> _client{ nullptr };

    // prevent copy/move
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;
    Client(Client&&) = delete;
    Client& operator=(Client&&) = delete;
};
