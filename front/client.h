#pragma once

#include <cstdint>
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
    Client(bool& create_ok) noexcept;
    ~Client() noexcept = default;

    std::string error_page_404() const noexcept;
    std::string error_page_403() const noexcept;
    static std::string generic_error(int error, const std::string& message) noexcept;

    std::pair<std::string, std::string> static_file(const std::string& file) const noexcept;

    std::string home_page() const noexcept;
    std::string dashboard_page() const noexcept;

    std::string login_page() const noexcept;

    std::string confirm_action_page() const noexcept;

    std::string user_list_page() const noexcept;
    std::string add_user_page() const noexcept;
    std::string update_user_page() const noexcept;

    std::string admin_list_page() const noexcept;

    std::string video_list_page() const noexcept;
    std::string add_video_page() const noexcept;
    std::string update_video_page() const noexcept;
    std::string watch_video_page() const noexcept;

    std::vector<std::string> video_list() const noexcept;
    std::vector<std::string> video_list(const std::string& user_id) const noexcept;
    std::vector<std::string> video_list(const std::string& user_id, const std::string& search) const noexcept;
    std::vector<std::string> no_right_video_list() const noexcept;
    std::vector<std::string> no_right_video_list(const std::string& search) const noexcept;

    std::string video_title(const std::string& video_id) const noexcept;
    std::int64_t video_views(const std::string& video_id) const noexcept;

    bool is_admin(const std::string& user_id) const noexcept;
    bool is_super_admin(const std::string& user_id) const noexcept;
    bool is_user(const std::string& user_id) const noexcept;
    std::string user_name(const std::string& user_id) const noexcept;
    std::string user_id(const std::string& username) const noexcept;
    void add_admin(const std::string& username, const std::string& password) const noexcept;
    void add_user(const std::string& username, const std::string& password) const noexcept;
    void update_user(const std::string& user_id, const std::string& password) const noexcept;
    void delete_user(const std::string& user_id) const noexcept;
    bool is_valid_user(const std::string& user_id, const std::string& password) const noexcept;

    std::int64_t user_count() const noexcept;
    std::int64_t video_count() const noexcept;
    std::int64_t view_count() const noexcept;

    std::vector<std::string> user_list() const noexcept;
    std::vector<std::string> admin_list() const noexcept;

    void add_video(const std::string& title, const std::string& content, const std::vector<std::string>& allowed_user_ids) const noexcept;
    void update_video(const std::string& video_id, const std::vector<std::string>& allowed_user_ids) const noexcept;
    void delete_video(const std::string& video_id) const noexcept;
    void increment_video_views(const std::string& video_id) const noexcept;
    bool video(const std::string& video_id, const std::string& range_header, const std::function<bool(const char*, std::size_t)>& callback) const noexcept;
    std::int64_t video_size(const std::string& video_id) const noexcept;
    std::string thumbnail(const std::string& video_id) const noexcept;
    bool has_video_right(const std::string& video_id) const noexcept;
    bool has_video_right(const std::string& video_id, const std::string& user_id) const noexcept;

    std::vector<std::string> video_right_list(const std::string& video_id) const noexcept;

private:
    std::unique_ptr<httplib::Client> _client{ nullptr };

    // prevent copy/move
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;
    Client(Client&&) = delete;
    Client& operator=(Client&&) = delete;
};
