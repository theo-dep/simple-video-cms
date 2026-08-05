#include "session.h"

#include "cookie.h"
#include "crypto.h"

#include <format>
#include <mutex>

namespace session
{
    constexpr const char* cookie_key() { return "id"; }
}

const std::string& Session::create_session(int user_id, std::chrono::seconds max_age)
{
    clean_expired_sessions();

    const std::unique_lock lock(_mutex);
    const std::string session_id{ generate_session_id(user_id) };
    _sessions[session_id].user_id = user_id;
    _sessions[session_id].max_age = max_age;

    return _sessions.find(session_id)->first;
}

int Session::user_from_session(const std::string& session_id) const
{
    const std::shared_lock lock(_mutex);

    const auto it_session{ _sessions.find(session_id) };
    if (it_session == _sessions.cend()) {
        return invalid_user_id(); // Session not found
    }

    return it_session->second.user_id;
}

std::string Session::remove_session_reset_cookie(const std::string& session_id)
{
    clean_expired_sessions();

    const std::unique_lock lock(_mutex);
    _sessions.erase(session_id);

    return cookie::insert_to_cookie(session::cookie_key(), std::string{}, std::chrono::seconds{ 0 });
}

std::string Session::extract_session_id_from_cookie(const std::string& cookie)
{
    return cookie::value_from_cookie(cookie, session::cookie_key());
}

std::string Session::insert_session_id_to_cookie(const std::string& session_id)
{
    const std::unique_lock lock(_mutex);

    const auto it_session_data{ _sessions.find(session_id) };
    if (it_session_data == _sessions.end()) {
        return {};
    }

    it_session_data->second.creation = std::chrono::system_clock::now();
    return cookie::insert_to_cookie(session::cookie_key(), session_id, it_session_data->second.max_age);
}

void Session::clean_expired_sessions()
{
    const std::unique_lock lock(_mutex);
    std::erase_if(_sessions,
                  [now{ std::chrono::system_clock::now() }](const std::pair<std::string, Data>& session) {
                      return (now - session.second.creation > session.second.max_age);
                  });
}

std::string Session::generate_session_id(int user_id)
{
    return crypto::sha512(std::format("sess_{}_{}_ion", crypto::random_string(), user_id));
}
