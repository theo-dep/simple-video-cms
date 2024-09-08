#pragma once

#include <httplib.hpp>

class Server : protected httplib::Server
{
public:
    Server();

    int start();

protected:
    void serve_template();

    void serve_most_viewed();

    void serve_video_title();
    void serve_video_views();
    void serve_video_uploader();

    void serve_is_admin();
};
