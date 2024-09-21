#include "session.h"

#include <cstring>

std::string Session::create_session(const std::string& username) noexcept
{
    const std::lock_guard<std::mutex> lock(_mutex);
    const std::string session_id{ generate_session_id() };
    _sessions.insert({ session_id, username });
    return session_id;
}

std::string Session::user_from_session(const std::string& session_id) const noexcept
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _sessions.find(session_id);
    if (it != _sessions.end()) {
        return it->second;
    }
    return ""; // Session not found
}

void Session::remove_session(const std::string& session_id) noexcept
{
    std::lock_guard<std::mutex> lock(_mutex);
    _sessions.erase(session_id);
}

bool Session::is_valid_session(const std::string& session_id) const noexcept
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _sessions.find(session_id) != _sessions.end();
}

bool Session::is_valid_session_from_cookie(const std::string& cookie) const noexcept
{
    const std::string session_id{ Session::extract_session_id_from_cookie(cookie) };
    return is_valid_session(session_id);
}

namespace session
{
    constexpr const char* cookie_key() { return "Session-ID="; }
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
