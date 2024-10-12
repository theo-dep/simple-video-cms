#pragma once

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
    std::string watch_video_page() const noexcept;

    std::vector<std::string> video_list() const noexcept;
    std::vector<std::string> most_viewed_video_list() const noexcept;

    std::string video_title(const std::string& id) const noexcept;
    int video_views(const std::string& id) const noexcept;

    bool is_admin(const std::string& username) const noexcept;
    bool is_super_admin(const std::string& username) const noexcept;
    bool is_user(const std::string& username) const noexcept;
    void add_admin(const std::string& username, const std::string& password) const noexcept;
    void add_user(const std::string& username, const std::string& password) const noexcept;
    void update_admin(const std::string& username, const std::string& password) const noexcept;
    void update_user(const std::string& username, const std::string& password) const noexcept;
    void delete_admin(const std::string& username) const noexcept;
    void delete_user(const std::string& username) const noexcept;
    bool is_valid_user(const std::string& username, const std::string& password) const noexcept;

    int user_count() const noexcept;
    int video_count() const noexcept;
    int view_count() const noexcept;

    std::vector<std::string> user_list() const noexcept;
    std::vector<std::string> admin_list() const noexcept;

    void add_video(const std::string& title, const std::string& content) const noexcept;
    void delete_video(const std::string& id) const noexcept;
    void increment_video_views(const std::string& id) const noexcept;
    std::string video(const std::string& id) const noexcept;

private:
    std::unique_ptr<httplib::Client> _client{ nullptr };

    // prevent copy/move
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;
    Client(Client&&) = delete;
    Client& operator=(Client&&) = delete;
};
