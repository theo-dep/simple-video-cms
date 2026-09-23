#include <videosession.h>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("VideoSession::add_session")
{
    SECTION("creates a distinct session per call")
    {
        VideoSession video_session;

        const std::string session1{ video_session.add_session("v1") };
        const std::string session2{ video_session.add_session("v1") };

        REQUIRE(session1 != session2);
        REQUIRE(video_session.validate_segment_access(session1, "v1", "video_000.ts"));
        REQUIRE(video_session.validate_segment_access(session2, "v1", "video_000.ts"));
    }
}

TEST_CASE("VideoSession::validate_segment_access")
{
    SECTION("rejects access when session does not exist")
    {
        VideoSession video_session;

        REQUIRE_FALSE(video_session.validate_segment_access("s1", "v1", "video_000.ts"));
    }

    SECTION("accepts valid sequence after session creation")
    {
        VideoSession video_session;
        const std::string session{ video_session.add_session("v1") };

        REQUIRE(video_session.validate_segment_access(session, "v1", "video_000.ts"));
        REQUIRE(video_session.validate_segment_access(session, "v1", "video_001.ts"));
    }

    SECTION("rejects malformed segment name")
    {
        VideoSession video_session;
        const std::string session{ video_session.add_session("v1") };

        REQUIRE_FALSE(video_session.validate_segment_access(session, "v1", "bad-segment"));
    }

    SECTION("bans suspicious start when too many prefetch segments")
    {
        VideoSession video_session;
        const std::string session{ video_session.add_session("v1") };

        REQUIRE(video_session.validate_segment_access(session, "v1", "video_000.ts"));
        REQUIRE(video_session.validate_segment_access(session, "v1", "video_001.ts"));
        REQUIRE(video_session.validate_segment_access(session, "v1", "video_002.ts"));
        REQUIRE(video_session.validate_segment_access(session, "v1", "video_003.ts"));
        REQUIRE(video_session.validate_segment_access(session, "v1", "video_004.ts"));

        REQUIRE_FALSE(video_session.validate_segment_access(session, "v1", "video_005.ts"));
    }

    SECTION("bans repeated large forward jumps")
    {
        VideoSession video_session;
        const std::string session{ video_session.add_session("v1") };
        video_session.start_session(session, "v1");

        REQUIRE(video_session.validate_segment_access(session, "v1", "video_000.ts"));
        REQUIRE(video_session.validate_segment_access(session, "v1", "video_010.ts"));
        REQUIRE(video_session.validate_segment_access(session, "v1", "video_020.ts"));

        REQUIRE_FALSE(video_session.validate_segment_access(session, "v1", "video_030.ts"));
    }

    SECTION("bans repeated large backward jumps")
    {
        VideoSession video_session;
        const std::string session{ video_session.add_session("v1") };
        video_session.start_session(session, "v1");

        REQUIRE(video_session.validate_segment_access(session, "v1", "video_010.ts"));
        REQUIRE(video_session.validate_segment_access(session, "v1", "video_007.ts"));
        REQUIRE(video_session.validate_segment_access(session, "v1", "video_004.ts"));

        REQUIRE_FALSE(video_session.validate_segment_access(session, "v1", "video_001.ts"));
    }

    SECTION("does not let a session access segments of another video")
    {
        VideoSession video_session;
        const std::string session{ video_session.add_session("v1") };

        REQUIRE_FALSE(video_session.validate_segment_access(session, "v2", "video_000.ts"));
    }
}

TEST_CASE("VideoSession::reset_session")
{
    SECTION("resets segment history and allows playback from beginning")
    {
        VideoSession video_session;
        const std::string session{ video_session.add_session("v1") };
        video_session.start_session(session, "v1");

        REQUIRE(video_session.validate_segment_access(session, "v1", "video_000.ts"));
        REQUIRE(video_session.validate_segment_access(session, "v1", "video_010.ts"));

        video_session.reset_session(session, "v1");

        REQUIRE(video_session.validate_segment_access(session, "v1", "video_000.ts"));
    }
}

TEST_CASE("VideoSession::clear_session")
{
    SECTION("removes the session and rejects further segment access")
    {
        VideoSession video_session;
        const std::string session{ video_session.add_session("v1") };
        video_session.start_session(session, "v1");

        REQUIRE(video_session.validate_segment_access(session, "v1", "video_000.ts"));

        video_session.clear_session(session, "v1");

        REQUIRE_FALSE(video_session.validate_segment_access(session, "v1", "video_001.ts"));
    }

    SECTION("does nothing when session does not exist")
    {
        VideoSession video_session;

        video_session.clear_session("s1", "v1");

        REQUIRE_FALSE(video_session.validate_segment_access("s1", "v1", "video_000.ts"));
    }

    SECTION("clears only the targeted video session")
    {
        VideoSession video_session;
        const std::string session1{ video_session.add_session("v1") };
        const std::string session2{ video_session.add_session("v2") };

        video_session.clear_session(session1, "v1");

        REQUIRE_FALSE(video_session.validate_segment_access(session1, "v1", "video_000.ts"));
        REQUIRE(video_session.validate_segment_access(session2, "v2", "video_000.ts"));
    }
}
