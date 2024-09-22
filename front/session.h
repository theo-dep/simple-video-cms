#pragma once

#include <mutex>
#include <string>
#include <unordered_map>

class Session
{
public:
    // Constructor
    Session() noexcept = default;

    // Create a new session for a user
    std::string create_session(const std::string& username) noexcept;

    // Get the username associated with a session ID
    std::string user_from_session(const std::string& session_id) const noexcept;

    // Remove a session
    void remove_session(const std::string& session_id) noexcept;

    // Check if a session is valid
    bool is_valid_session(const std::string& session_id) const noexcept;

    // Check if a session is valid from cookie header
    bool is_valid_session_from_cookie(const std::string& cookie) const noexcept;

    // res.set_header("Set-Cookie", "Session-ID=" + session_id + "; HttpOnly");
    static std::string extract_session_id_from_cookie(const std::string& cookie) noexcept;

    // res.set_header("Set-Cookie", "Session-ID=" + session_id + "; HttpOnly");
    static std::string insert_session_id_to_cookie(const std::string& session_id) noexcept;

private:
    // Generate a unique session ID (for simplicity, using a counter)
    static std::string generate_session_id() noexcept;

    std::unordered_map<std::string, std::string> _sessions; // session_id -> username
    mutable std::mutex _mutex;

    // prevent copy/move
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;
    Session(Session&&) = delete;
    Session& operator=(Session&&) = delete;
};
