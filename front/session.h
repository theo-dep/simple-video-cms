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
    std::string create_session(const std::string& username);

    // Get the username associated with a session ID
    std::string user_from_session(const std::string& session_id) const;

    // Remove a session
    void remove_session(const std::string& session_id);

    // Check if a session is valid
    bool is_valid_session(const std::string& session_id) const;

    // res.set_header("Set-Cookie", "Session-ID=" + session_id + "; HttpOnly");
    static std::string extract_session_id_from_cookie(const std::string& cookie);

private:
    // Generate a unique session ID (for simplicity, using a counter)
    static std::string generate_session_id();

    std::unordered_map<std::string, std::string> _sessions; // session_id -> username
    mutable std::mutex _mutex;

    // prevent copy/move
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;
    Session(Session&&) = delete;
    Session& operator=(Session&&) = delete;
};
