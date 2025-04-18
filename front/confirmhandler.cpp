#include "confirmhandler.h"

#include "stringutils.h"

#include <httplib.h>

std::size_t ConfirmHandler::SignalHash::operator()(const std::pair<std::string, bool>& key) const
{
    const auto& [confirm_signal_str, confirm]{ key };
    const std::size_t h1{ hash_type{}(confirm_signal_str) };
    const std::size_t h2{ hash_type{}(su::bool_to_string(confirm)) };
    return h1 ^ (h2 << 1);
}

std::size_t ConfirmHandler::SignalHash::operator()(const std::pair<std::shared_ptr<const ConfirmHandler::Signal>, bool>& key) const
{
    const auto& [confirm_signal, confirm]{ key };
    return operator()(std::make_pair(confirm_signal->to_string(), confirm));
}

bool operator==(const std::string& str, const std::shared_ptr<const ConfirmHandler::Signal>& signal)
{
    return (str == signal->to_string());
}

struct ConfirmHandler::Signal::Private
{
    explicit Private() = default;
};

ConfirmHandler::Signal::Signal(Private /*unused*/, ConfirmHandler& handler)
    : _handler{ handler }
{
}

ConfirmHandler::Signal& ConfirmHandler::Signal::on_confirm(const std::function<void(httplib::Response&)>& handle)
{
    _handler._handle_map.insert_or_assign(std::make_pair(ptr(), true), [handle](httplib::Response& res) {
        if (handle)
            handle(res);
    });
    return *this;
}

ConfirmHandler::Signal& ConfirmHandler::Signal::on_deny(const std::function<void(httplib::Response&)>& handle)
{
    _handler._handle_map.insert_or_assign(std::make_pair(ptr(), false), [handle](httplib::Response& res) {
        if (handle)
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
    const ConnectionHandleMap::const_iterator it_handle{ _handle_map.find(std::make_pair(confirm_signal_str, confirm)) };
    if (it_handle != _handle_map.cend() && it_handle->second)
        it_handle->second(res);

    std::erase_if(_handle_map, [&confirm_signal_str](const ConnectionHandleMap::value_type& item) -> bool {
        const auto& [key, value]{ item };
        const auto& [confirm_signal, confirm]{ key };
        return (confirm_signal_str == confirm_signal);
    });
}
