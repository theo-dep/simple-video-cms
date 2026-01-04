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
    constexpr const char* not_logged_user_id() { return "not_logged_user_id"; }
}

const std::string& Session::create_session(const std::string& user_id)
{
    const std::string session_id{ generate_crypt_session_id(user_id) };
    insert_value_from_session(session_id, session::user_id_key(), user_id);

    const std::scoped_lock lock(_mutex);
    return _sessions.find(session_id)->first;
}

const std::string& Session::create_not_logged_session()
{
    const std::string session_id{ generate_session_id(session::not_logged_user_id()) };
    insert_value_from_session(session_id, session::user_id_key(), session::not_logged_user_id());

    const std::scoped_lock lock(_mutex);
    return _sessions.find(session_id)->first;
}

const std::string& Session::user_from_session(const std::string& session_id) const
{
    return value_from_session(session_id, session::user_id_key());
}

const std::string& Session::value_from_session(const std::string& session_id, const std::string& key) const
{
    const std::scoped_lock lock(_mutex);
    static const std::string empty_string;

    const auto it_session_data{ _sessions.find(session_id) };
    if (it_session_data == _sessions.cend()) {
        return empty_string; // Session not found
    }

    const auto it_session_value{ it_session_data->second.find(key) };
    if (it_session_value == it_session_data->second.cend()) {
        return empty_string; // Key not found
    }
    return it_session_value->second;
}

void Session::insert_value_from_session(const std::string& session_id, const std::string& key, const std::string& value)
{
    const std::scoped_lock lock(_mutex);
    _sessions[session_id][key] = value;
}

void Session::remove_value_from_session(const std::string& session_id, const std::string& key)
{
    const std::scoped_lock lock(_mutex);

    const auto it_session_data{ _sessions.find(session_id) };
    if (it_session_data == _sessions.cend()) {
        return;
    }

    it_session_data->second.erase(key);
}

const std::unordered_map<std::string, std::string>& Session::values_from_session(const std::string& session_id) const
{
    const std::scoped_lock lock(_mutex);

    const auto it_session_data{ _sessions.find(session_id) };
    if (it_session_data == _sessions.cend()) {
        static const std::unordered_map<std::string, std::string> dummy;
        return dummy;
    }

    return it_session_data->second;
}

void Session::remove_session(const std::string& session_id)
{
    const std::scoped_lock lock(_mutex);
    _sessions.erase(session_id);
}

bool Session::is_valid_session(const std::string& session_id) const
{
    const std::scoped_lock lock(_mutex);
    return _sessions.contains(session_id) && !session_id.contains(session::not_logged_user_id());
}

bool Session::is_not_logged_session(const std::string& session_id) const
{
    const std::scoped_lock lock(_mutex);
    return _sessions.contains(session_id) && session_id.contains(session::not_logged_user_id());
}

bool Session::is_valid_session_from_cookie(const std::string& cookie) const
{
    const std::string session_id{ extract_session_id_from_cookie(cookie) };
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
    return ("sess_" + crypto::random_string() + "_" + user_id + "_ion");
}

std::string Session::generate_crypt_session_id(const std::string& user_id)
{
    return crypto::sha512(generate_session_id(user_id));
}
