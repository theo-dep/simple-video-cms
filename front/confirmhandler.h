#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace httplib
{
    struct Response;
}

class ConfirmHandler
{
    friend struct Signal;

public:
    class Signal : public std::enable_shared_from_this<Signal>
    {
        struct Private;
        friend class ConfirmHandler;

    public:
        Signal(Private, ConfirmHandler& handler);

        // called on confirm true
        Signal& on_confirm(const std::function<void(httplib::Response&)>& handle);
        // called on confirm false
        Signal& on_deny(const std::function<void(httplib::Response&)>& handle);
        // transform to string to be shared by requests
        std::string to_string() const;

    private:
        std::shared_ptr<const Signal> ptr() const;

        ConfirmHandler& _handler;
    };

    ConfirmHandler() = default;
    ~ConfirmHandler() = default;

    // create a ConfirmConnection to connect after to a confirm/deny handle
    std::shared_ptr<Signal> create();

    // send the confirmation to previously connected handles (transformed to string)
    // exception must be catched by httplib Server::set_exception_handler
    void confirm(httplib::Response& res, const std::string& confirm_signal_str, bool confirm);

private:
    struct SignalHash
    {
        using hash_type = std::hash<std::string>;
        using is_transparent = void;

        std::size_t operator()(const std::pair<std::string, bool>& key) const;
        std::size_t operator()(const std::pair<std::shared_ptr<const ConfirmHandler::Signal>, bool>& key) const;
    };

    using ConnectionHandle = std::function<void(httplib::Response&)>;
    using ConnectionHandleMap = std::unordered_map<std::pair<std::shared_ptr<const Signal>, bool>, ConnectionHandle, SignalHash, std::equal_to<>>;
    ConnectionHandleMap _handle_map;
    mutable std::mutex _mutex;

    // prevent copy/move
    ConfirmHandler(const ConfirmHandler&) = delete;
    ConfirmHandler& operator=(const ConfirmHandler&) = delete;
    ConfirmHandler(ConfirmHandler&&) = delete;
    ConfirmHandler& operator=(ConfirmHandler&&) = delete;
};
