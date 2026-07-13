#pragma once

#include <chrono>
#include <shared_mutex>
#include <string>
#include <unordered_map>

class VideoSession
{
public:
    // Constructor
    VideoSession() = default;

    void add_session(const std::string& session_id, const std::string& video_id);
    void start_session(const std::string& session_id, const std::string& video_id);
    void reset_session(const std::string& session_id, const std::string& video_id);

    bool validate_segment_access(const std::string& session_id, const std::string& video_id, const std::string& segment);

private:
    static bool is_expired(const std::chrono::system_clock::time_point& time);
    void clean_expired_sessions(const std::string& except_session_id);

    static constexpr std::chrono::seconds duration{ 300 };
    static constexpr int not_started_segment_number{ 3 };
    static constexpr int max_segment_number{ 3 };
    static constexpr int min_segment_number{ 2 };
    static constexpr int max_sink_number{ 1 };

    struct State
    {
        bool started{ false };                                                                // player started
        bool banned{ false };                                                                 // played banned
        int last_segment{ -1 };                                                               // last segment served (-1 = none)
        int sink_count{ 0 };                                                                  // number of sink for this session
        std::chrono::system_clock::time_point created_at{ std::chrono::system_clock::now() }; // creation session timestamp
    };
    friend struct std::formatter<State, char>;

    struct Key
    {
        std::string session_id;
        std::string video_id;

        bool operator==(const Key&) const = default;
    };
    friend struct std::formatter<Key, char>;

    struct KeyHash
    {
        std::size_t operator()(const Key& key) const;
    };

    std::unordered_map<Key, State, KeyHash> _sessions; // session_id, video_id -> state
    mutable std::shared_mutex _mutex;

    // prevent copy/move
    VideoSession(const VideoSession&) = delete;
    VideoSession& operator=(const VideoSession&) = delete;
    VideoSession(VideoSession&&) = delete;
    VideoSession& operator=(VideoSession&&) = delete;
};