#include "videosession.h"

#include "logging.h"
#include "stringutils.h"

template <>
struct std::formatter<VideoSession::Key> : std::formatter<std::string>
{
    template <class FmtContext>
    FmtContext::iterator format(const VideoSession::Key& key, FmtContext& ctx) const
    {
        return std::format_to(ctx.out(), "[ session_id={}, video_id={} ]", key.session_id, key.video_id);
    }
};

template <>
struct std::formatter<VideoSession::State> : std::formatter<std::string>
{
    template <class FmtContext>
    FmtContext::iterator format(const VideoSession::State& state, FmtContext& ctx) const
    {
        return std::format_to(ctx.out(),
                              "[ started={}, banned={}, last_segment={}, sink_count={}, create_at={} ]",
                              state.started, state.banned, state.last_segment, state.sink_count, state.created_at);
    }
};

void VideoSession::add_session(const std::string& session_id, const std::string& video_id)
{
    clean_expired_sessions({});

    const std::scoped_lock lock(_mutex);
    const Key key{ .session_id = session_id, .video_id = video_id };
    State& state{ _sessions[key] };
    state = State{}; // reset

    logging::debug{ "new session created {} <=> {}", key, state };
}

void VideoSession::start_session(const std::string& session_id, const std::string& video_id)
{
    clean_expired_sessions({});

    const std::scoped_lock lock(_mutex);

    const auto session{ _sessions.find({ session_id, video_id }) };
    if (session == _sessions.end()) {
        return;
    }

    session->second.started = true;
    logging::debug{ "session started {} <=> {}", session->first, session->second };
}

void VideoSession::reset_session(const std::string& session_id, const std::string& video_id)
{
    clean_expired_sessions({});

    const std::scoped_lock lock(_mutex);

    const auto session{ _sessions.find({ session_id, video_id }) };
    if (session == _sessions.end()) {
        return;
    }

    session->second.sink_count = 0;
    session->second.last_segment = -1;
}

bool VideoSession::validate_segment_access(const std::string& session_id, const std::string& video_id, const std::string& segment)
{
    clean_expired_sessions(session_id);

    // extract the segment number from "video_042.ts"
    const std::size_t underscore{ segment.rfind('_') };
    const std::size_t dot{ segment.rfind('.') };
    if (underscore == std::string::npos || dot == std::string::npos) {
        logging::error{ "fail to find segment number in {} for video {}", segment, video_id };
        return false;
    }

    const int segment_number{ su::string_to_int(segment.substr(underscore + 1, dot - underscore - 1)) };

    const std::scoped_lock lock(_mutex);

    const auto session{ _sessions.find({ session_id, video_id }) };
    if (session == _sessions.end()) {
        logging::error{ "session not found {} for {}", session_id, video_id };
        return false;
    }

    // player banned from previous attempt
    if (session->second.banned) {
        logging::info{ "player banned {} <=> {}", session->first, session->second };
        return false;
    }

    // reload needed if hang more than duration
    if (is_expired(session->second.created_at)) {
        session->second.banned = true;
        logging::info{ "session expired (now={}) {} <=> {}", std::chrono::system_clock::now(), session->first, session->second };
        return false;
    }

    // player not started but allow prefetch
    if (session->second.last_segment > not_started_segment_number && !session->second.started) {
        session->second.banned = true;
        logging::info{ "suspicious start (requested={}) {} <=> {}", segment_number, session->first, session->second };
        return false;
    }

    // no jump forward
    if (session->second.last_segment >= 0 && segment_number > session->second.last_segment + max_segment_number) {
        if (session->second.sink_count > max_sink_number) {
            session->second.banned = true;
            logging::info{ "suspicious jump (requested={}) {} <=> {}", segment_number, session->first, session->second };
            return false;
        }

        ++(session->second.sink_count);
    }

    // no return back
    if (session->second.last_segment >= 0 && segment_number < session->second.last_segment - min_segment_number) {
        if (session->second.sink_count > max_sink_number) {
            session->second.banned = true;
            logging::info{ "suspicious back return (requested={}) {} <=> {}", segment_number, session->first, session->second };
            return false;
        }

        ++(session->second.sink_count);
    }

    session->second.last_segment = segment_number;
    return true;
}

bool VideoSession::is_expired(const std::chrono::system_clock::time_point& time)
{
    const std::chrono::system_clock::time_point now{ std::chrono::system_clock::now() };
    return (now - time > duration);
}

void VideoSession::clean_expired_sessions(const std::string& except_session_id)
{
    const std::scoped_lock lock(_mutex);
    std::erase_if(_sessions,
                  [&except_session_id](const decltype(_sessions)::value_type& session) {
                      return except_session_id != session.first.session_id && is_expired(session.second.created_at);
                  });
}

std::size_t VideoSession::KeyHash::operator()(const Key& key) const
{
    const std::size_t h1{ std::hash<std::string>{}(key.session_id) };
    const std::size_t h2{ std::hash<std::string>{}(key.video_id) };
    return h1 ^ (h2 << 1);
}
