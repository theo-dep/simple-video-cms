#include "confirmhandler.h"

#include <httplib.h>

std::size_t ConfirmHandler::SignalHash::operator()(const std::string& confirm_signal_str) const
{
    return hash_type{}(confirm_signal_str);
}

std::size_t ConfirmHandler::SignalHash::operator()(const std::shared_ptr<const ConfirmHandler::Signal>& confirm_signal) const
{
    return hash_type{}(confirm_signal->to_string());
}

bool operator==(const std::string& str, const std::shared_ptr<const ConfirmHandler::Signal>& signal)
{
    return (str == signal->to_string());
}

struct ConfirmHandler::Signal::Private
{
    explicit Private() = default;
};

ConfirmHandler::Signal::Signal(Private, ConfirmHandler& handler)
    : _handler{ handler }
{
}

ConfirmHandler::Signal& ConfirmHandler::Signal::on_confirm(const std::function<void(httplib::Response&)>& handle)
{
    _handler._handle_map.emplace(ptr(), [handle, signal_ptr{ ptr() }](httplib::Response& res, const std::string& confirm_signal_str, bool confirm) {
        if (confirm_signal_str == signal_ptr && confirm && handle)
            handle(res);
    });
    return *this;
}

ConfirmHandler::Signal& ConfirmHandler::Signal::on_deny(const std::function<void(httplib::Response&)>& handle)
{
    _handler._handle_map.emplace(ptr(), [handle, signal_ptr{ ptr() }](httplib::Response& res, const std::string& confirm_signal_str, bool confirm) {
        if (confirm_signal_str == signal_ptr && !confirm && handle)
            handle(res);
    });
    return *this;
}

std::string ConfirmHandler::Signal::to_string() const
{
    // flush the signal address
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return std::to_string(reinterpret_cast<std::uintptr_t>(this));
}

std::shared_ptr<const ConfirmHandler::Signal> ConfirmHandler::Signal::ptr() const
{
    return shared_from_this();
}

std::shared_ptr<ConfirmHandler::Signal> ConfirmHandler::create()
{
    return std::make_shared<Signal>(Signal::Private{}, *this);
}

void ConfirmHandler::confirm(httplib::Response& res, const std::string& confirm_signal_str, bool confirm)
{
    const auto [it_first_confirm_signal, it_last_confirm_signal]{ _handle_map.equal_range(confirm_signal_str) };
    std::for_each(it_first_confirm_signal, it_last_confirm_signal,
                  [&](const ConnectionHandleMap::value_type& item) {
                      const auto& [confirm_signal, connection_handle]{ item };
                      connection_handle(res, confirm_signal_str, confirm);
                  });

    _handle_map.erase(it_first_confirm_signal, it_last_confirm_signal);
}
