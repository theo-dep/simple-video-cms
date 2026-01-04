#pragma once

#include <mutex>
#include <string>
#include <unordered_map>

class Session
{
public:
    // Constructor
    Session() = default;

    // Create a new session for a user
    const std::string& create_session(const std::string& user_id);
    const std::string& create_not_logged_session();

    // Get the user_id associated with a session ID
    const std::string& user_from_session(const std::string& session_id) const;

    // Access to key, value map for a session_id
    const std::string& value_from_session(const std::string& session_id, const std::string& key) const;
    void insert_value_from_session(const std::string& session_id, const std::string& key, const std::string& value);
    void remove_value_from_session(const std::string& session_id, const std::string& key);
    const std::unordered_map<std::string, std::string>& values_from_session(const std::string& session_id) const;

    // Remove a session
    void remove_session(const std::string& session_id);

    // Check if a session is valid
    bool is_valid_session(const std::string& session_id) const;
    bool is_not_logged_session(const std::string& session_id) const;

    // Check if a session is valid from cookie header
    bool is_valid_session_from_cookie(const std::string& cookie) const;

    // res.set_header("Set-Cookie", "Session-ID=" + session_id + "; HttpOnly");
    static std::string extract_session_id_from_cookie(const std::string& cookie);

    // res.set_header("Set-Cookie", "Session-ID=" + session_id + "; HttpOnly");
    static std::string insert_session_id_to_cookie([[maybe_unused]] const std::string& url, const std::string& session_id);

private:
    // Generate a unique session ID (using a counter, a user_id and crypto sha512)
    static std::string generate_session_id(const std::string& user_id);
    static std::string generate_crypt_session_id(const std::string& user_id);

    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> _sessions; // session_id -> key, value
    mutable std::mutex _mutex;

    // prevent copy/move
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;
    Session(Session&&) = delete;
    Session& operator=(Session&&) = delete;
};
