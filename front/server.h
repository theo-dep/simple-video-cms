#pragma once

#include "session.h"

#include <httplib.h>
#include <inja.hpp>

class Server : protected httplib::Server
{
public:
    Server() noexcept;

    int start() noexcept;

protected:
    void set_no_cache_headers(httplib::Response& res) noexcept;

    void set_error_handler() noexcept;
    void set_exception_handler() noexcept;
    void set_logger() noexcept;

    void serve_home() noexcept;

    void serve_login() noexcept;
    void serve_logout() noexcept;

protected:
    Session _session;

    inja::Environment _env;
};
