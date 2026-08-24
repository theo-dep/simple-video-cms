#pragma once

#include <chrono>
#include <functional>
#include <shared_mutex>
#include <string>
#include <unordered_map>

class Session
{
public:
    Session() = default;
    ~Session() = default;

    // prevent copy/move
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;
    Session(Session&&) = delete;
    Session& operator=(Session&&) = delete;

    // From serialization
    void init_from_map(const std::vector<std::tuple<std::string, int, std::string, std::string>>& data);

    // To serialization
    void add_insert_function(std::function<void(const std::string&, int, const std::string&, const std::string&)> function) { _insert_function = std::move(function); }
    void add_remove_function(std::function<void(const std::string&)> function) { _remove_function = std::move(function); }

    // Create a new session for a user
    static constexpr auto default_session_max_age = std::chrono::days{ 30 };
    const std::string& create_session(int user_id, std::chrono::seconds max_age = default_session_max_age);

    // Get the user_id associated with a session ID or invalid_user_id()
    int user_from_session(const std::string& session_id) const;

    // user_from_session value if no session ID
    static constexpr int invalid_user_id() { return -1; }

    // Remove a session
    // res.set_header("Set-Cookie", "Session-ID=; HttpOnly");
    std::string remove_session_reset_cookie(const std::string& session_id);

    // res.set_header("Cookie", "Session-ID=" + session_id);
    static std::string extract_session_id_from_cookie(const std::string& cookie);

    // res.set_header("Set-Cookie", "Session-ID=" + session_id + "; HttpOnly");
    std::string insert_session_id_to_cookie(const std::string& session_id);

private:
    void clean_expired_sessions();

    // Generate a unique session ID (using a counter, a user_id and crypto sha512)
    static std::string generate_session_id(int user_id);

    struct Data
    {
        int user_id;
        std::chrono::system_clock::time_point creation;
        std::chrono::seconds max_age;
    };

    std::unordered_map<std::string, Data> _sessions; // session_id -> key, value
    mutable std::shared_mutex _mutex;

    std::function<void(const std::string&, int, const std::string&, const std::string&)> _insert_function;
    std::function<void(const std::string&)> _remove_function;
};
