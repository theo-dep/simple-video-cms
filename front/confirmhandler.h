#pragma once

#include <functional>
#include <memory>
#include <string>

namespace httplib
{
    class Response;
}

class ConfirmHandler
{
    friend struct Signal;

public:
    struct Signal
    {
        // called on confirm true
        const Signal& on_confirm(const std::function<void(httplib::Response&)>& handle) const noexcept;
        // called on confirm false
        const Signal& on_deny(const std::function<void(httplib::Response&)>& handle) const noexcept;
        // transform to string to be shared by requests
        std::string to_string() const noexcept;

    private:
        friend class ConfirmHandler;
        Signal(const ConfirmHandler& handler) noexcept;

        const ConfirmHandler& _handler;
    };

    ConfirmHandler() noexcept;
    ~ConfirmHandler() noexcept;

    // create a ConfirmConnection to connect after to a confirm/deny handle
    const Signal& create() noexcept;

    // send the confirmation to previously connected handles (transformed to string)
    // exception must be catched by httplib Server::set_exception_handler
    void confirm(httplib::Response& res, const std::string& confirm_signal_str, bool confirm);

private:
    struct ConfirmHandlerImpl;
    std::unique_ptr<ConfirmHandlerImpl> _impl{ nullptr };

    // prevent copy/move
    ConfirmHandler(const ConfirmHandler&) = delete;
    ConfirmHandler& operator=(const ConfirmHandler&) = delete;
    ConfirmHandler(ConfirmHandler&&) = delete;
    ConfirmHandler& operator=(ConfirmHandler&&) = delete;
};
