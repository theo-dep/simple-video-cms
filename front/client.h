#pragma once

#include <httplib.hpp>

class Client : protected httplib::Client
{
public:
    Client();

    std::string get_404_error();
    std::string get_403_error();
    std::string get_generic_error(int error, const std::string& message);

    std::string get_homepage();

    std::vector<std::string> get_most_viewed();

    std::string video_title(const std::string& id);
    int video_views(const std::string& id);
    std::string video_uploader(const std::string& id);

    bool is_admin(const std::string& session_id);

protected:
    std::string get_page(const httplib::Result& res);
};
