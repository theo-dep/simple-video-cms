#include "confirmhandler.h"

#include <httplib.h>
#define KDBINDINGS_ENABLE_WARN_UNUSED
#include <kdbindings/signal.h>

#include <unordered_map>

struct SignalHash
{
    using hash_type = std::hash<std::string>;
    using is_transparent = void;

    std::size_t operator()(const std::string& confirm_signal_str) const
    {
        return hash_type{}(confirm_signal_str);
    }
    std::size_t operator()(const ConfirmHandler::Signal* confirm_signal) const
    {
        return hash_type{}(confirm_signal->to_string());
    }
};

bool operator==(const std::string& str, const ConfirmHandler::Signal* signal)
{
    return (str == signal->to_string());
}

struct ConfirmHandler::ConfirmHandlerImpl
{
    KDBindings::Signal<std::reference_wrapper<httplib::Response>, std::string, bool> signal;

    struct ConnectionHandle
    {
        KDBindings::ConnectionHandle confirm_handle;
        KDBindings::ConnectionHandle deny_handle;
    };
    std::unordered_map<const ConfirmHandler::Signal*, ConnectionHandle, SignalHash, std::equal_to<>> handle_map;
};

ConfirmHandler::ConfirmHandler()
    : _impl{ std::make_unique<ConfirmHandlerImpl>() }
{
}

ConfirmHandler::~ConfirmHandler()
{
    for (const auto& [confirm_signal, connection_handle] : _impl->handle_map)
        delete confirm_signal; // NOLINT(cppcoreguidelines-owning-memory): need to import gsl::owner<>
}

ConfirmHandler::Signal::Signal(const ConfirmHandler& handler)
    : _handler{ handler }
{
}

const ConfirmHandler::Signal& ConfirmHandler::Signal::on_confirm(const std::function<void(httplib::Response&)>& handle) const
{
    const KDBindings::ConnectionHandle confirm_handle{ _handler._impl->signal.connect(
        [handle, this](httplib::Response& res, const std::string& confirm_signal_str, bool confirm) {
            if (confirm_signal_str == this && confirm && handle)
                handle(res);
        }) };
    _handler._impl->handle_map[this].confirm_handle = confirm_handle;
    return *this;
}

const ConfirmHandler::Signal& ConfirmHandler::Signal::on_deny(const std::function<void(httplib::Response&)>& handle) const
{
    const KDBindings::ConnectionHandle deny_handle{ _handler._impl->signal.connect(
        [handle, this](httplib::Response& res, const std::string& confirm_signal_str, bool confirm) {
            if (confirm_signal_str == this && !confirm && handle)
                handle(res);
        }) };
    _handler._impl->handle_map[this].deny_handle = deny_handle;
    return *this;
}

std::string ConfirmHandler::Signal::to_string() const
{
    // flush the signal address
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return std::to_string(reinterpret_cast<std::uintptr_t>(this));
}

const ConfirmHandler::Signal& ConfirmHandler::create()
{
    return *(_impl->handle_map.insert({ new (std::nothrow) Signal(*this), ConfirmHandlerImpl::ConnectionHandle{} }).first->first);
}

void ConfirmHandler::confirm(httplib::Response& res, const std::string& confirm_signal_str, bool confirm)
{
    _impl->signal.emit(res, confirm_signal_str, confirm);

    const decltype(_impl->handle_map)::iterator it_confirm_signal{ _impl->handle_map.find(confirm_signal_str) };
    _impl->signal.disconnect(it_confirm_signal->second.confirm_handle);
    _impl->signal.disconnect(it_confirm_signal->second.deny_handle);
    delete it_confirm_signal->first; // NOLINT(cppcoreguidelines-owning-memory): need to import gsl::owner<>
    _impl->handle_map.erase(it_confirm_signal);
}
