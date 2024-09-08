#include "session.h"

std::string Session::create_session(const std::string& username)
{
    const std::lock_guard<std::mutex> lock(_mutex);
    const std::string session_id{ generate_session_id() };
    _sessions.insert({ session_id, username });
    return session_id;
}

std::string Session::user_from_session(const std::string& session_id) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _sessions.find(session_id);
    if (it != _sessions.end()) {
        return it->second;
    }
    return ""; // Session not found
}

void Session::remove_session(const std::string& session_id)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _sessions.erase(session_id);
}

bool Session::is_valid_session(const std::string& session_id) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _sessions.find(session_id) != _sessions.end();
}

std::string Session::extract_session_id_from_cookie(const std::string& cookie)
{
    static const std::string key("Session-ID=");

    std::string session_id;

    std::size_t start{ cookie.find(key) };
    if (start != std::string::npos) {
        start += key.length();
        std::size_t end{ cookie.find(';', start) };
        if (end == std::string::npos) {
            end = cookie.length();
        }
        session_id = cookie.substr(start, end - start);
    }

    return session_id;
}

std::string Session::generate_session_id()
{
    static int counter{ 0 };
    return "sess_" + std::to_string(counter++);
}
