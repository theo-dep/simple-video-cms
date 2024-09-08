#pragma once

#include "session.h"

#include <httplib.hpp>
#include <inja.hpp>

class Server : protected httplib::Server
{
public:
    Server();

    int start();

protected:
    void set_no_cache_headers(httplib::Response& res);

    void set_error_handler();
    void set_exception_handler();
    void set_logger();

    void serve_home();

protected:
    Session _session;

    inja::json _data;
    inja::Environment _env;
};
