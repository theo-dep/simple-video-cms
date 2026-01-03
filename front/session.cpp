#include "session.h"

#include "cookie.h"
#include "crypto.h"
#include "stringutils.h"

#include <chrono>
#include <cstring>

namespace session
{
    constexpr const char* cookie_key() { return "Session-ID"; }
    constexpr const char* user_id_key() { return "user_id"; }
}

const std::string& Session::create_session(const std::string& user_id)
{
    const std::string session_id{ generate_session_id(user_id) };
    insert_value_from_session(session_id, session::user_id_key(), user_id);

    const std::scoped_lock lock(_mutex);
    return _sessions.find(session_id)->first;
}

const std::string& Session::user_from_session(const std::string& session_id) const
{
    return value_from_session(session_id, session::user_id_key());
}

const std::string& Session::operator()(const std::string& session_id, const std::string& key) const
{
    return value_from_session(session_id, key);
}

const std::string& Session::value_from_session(const std::string& session_id, const std::string& key) const
{
    static const std::string empty_string;
    if (!is_valid_session(session_id)) {
        return empty_string; // Session not found
    }

    const std::scoped_lock lock(_mutex);
    if (!_sessions.at(session_id).contains(key)) {
        return empty_string; // Key not found
    }
    return _sessions.at(session_id).at(key);
}

void Session::insert_value_from_session(const std::string& session_id, const std::string& key, const std::string& value)
{
    const std::scoped_lock lock(_mutex);
    _sessions[session_id][key] = value;
}

void Session::remove_value_from_session(const std::string& session_id, const std::string& key)
{
    if (!is_valid_session(session_id)) {
        return;
    }

    const std::scoped_lock lock(_mutex);
    _sessions[session_id].erase(key);
}

void Session::remove_session(const std::string& session_id)
{
    const std::scoped_lock lock(_mutex);
    _sessions.erase(session_id);
}

bool Session::is_valid_session(const std::string& session_id) const
{
    const std::scoped_lock lock(_mutex);
    return _sessions.contains(session_id);
}

bool Session::is_valid_session_from_cookie(const std::string& cookie) const
{
    const std::string session_id{ Session::extract_session_id_from_cookie(cookie) };
    return is_valid_session(session_id);
}

std::string Session::extract_session_id_from_cookie(const std::string& cookie)
{
    return cookie::value_from_cookie(cookie, session::cookie_key());
}

std::string Session::insert_session_id_to_cookie([[maybe_unused]] const std::string& url, const std::string& session_id)
{
    constexpr std::chrono::days thirty_days{ 30 };
    constexpr std::chrono::seconds thirty_days_seconds{ thirty_days };
    return cookie::insert_to_cookie(url, session::cookie_key(), session_id, thirty_days_seconds);
}

std::string Session::generate_session_id(const std::string& user_id)
{
    return crypto::sha512("sess_" + crypto::random_string() + "_" + user_id + "_ion");
}
