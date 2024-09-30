#include "session.h"

#include <cstring>

namespace session
{
    constexpr const char* cookie_key() { return "Session-ID="; }
    constexpr const char* username_key() { return "username"; }
}

const std::string& Session::create_session(const std::string& username) noexcept
{
    const std::string session_id{ generate_session_id() };
    insert_value_from_session(session_id, session::username_key(), username);

    const std::lock_guard<std::mutex> lock(_mutex);
    return _sessions.find(session_id)->first;
}

const std::string& Session::user_from_session(const std::string& session_id) const noexcept
{
    return value_from_session(session_id, session::username_key());
}

const std::string& Session::operator()(const std::string& session_id, const std::string& key) const noexcept
{
    return value_from_session(session_id, key);
}

const std::string& Session::value_from_session(const std::string& session_id, const std::string& key) const noexcept
{
    static const std::string empty_string;
    if (!is_valid_session(session_id)) {
        return empty_string; // Session not found
    }

    const std::lock_guard<std::mutex> lock(_mutex);
    if (!_sessions.at(session_id).contains(key)) {
        return empty_string; // Key not found
    }
    return _sessions.at(session_id).at(key);
}

void Session::insert_value_from_session(const std::string& session_id, const std::string& key, const std::string& value) noexcept
{
    const std::lock_guard<std::mutex> lock(_mutex);
    _sessions[session_id][key] = value;
}

void Session::remove_value_from_session(const std::string& session_id, const std::string& key) noexcept
{
    if (!is_valid_session(session_id)) {
        return;
    }

    const std::lock_guard<std::mutex> lock(_mutex);
    _sessions[session_id].erase(key);
}

void Session::remove_session(const std::string& session_id) noexcept
{
    const std::lock_guard<std::mutex> lock(_mutex);
    _sessions.erase(session_id);
}

bool Session::is_valid_session(const std::string& session_id) const noexcept
{
    const std::lock_guard<std::mutex> lock(_mutex);
    return _sessions.contains(session_id);
}

bool Session::is_valid_session_from_cookie(const std::string& cookie) const noexcept
{
    const std::string session_id{ Session::extract_session_id_from_cookie(cookie) };
    return is_valid_session(session_id);
}

std::string Session::extract_session_id_from_cookie(const std::string& cookie) noexcept
{
    std::string session_id;

    std::size_t start{ cookie.find(session::cookie_key()) };
    if (start != std::string::npos) {
        start += std::strlen(session::cookie_key());
        std::size_t end{ cookie.find(';', start) };
        if (end == std::string::npos) {
            end = cookie.length();
        }
        session_id = cookie.substr(start, end - start);
    }

    return session_id;
}

std::string Session::insert_session_id_to_cookie(const std::string& session_id) noexcept
{
    return (session::cookie_key() + session_id + ";");
}

std::string Session::generate_session_id() noexcept
{
    static int counter{ 0 };
    return "sess_" + std::to_string(counter++);
}
