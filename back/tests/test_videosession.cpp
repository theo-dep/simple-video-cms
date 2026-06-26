#include <videosession.h>

#include <catch2/catch_test_macros.hpp>

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
        video_session.add_session("s1", "v1");

        REQUIRE(video_session.validate_segment_access("s1", "v1", "video_000.ts"));
        REQUIRE(video_session.validate_segment_access("s1", "v1", "video_001.ts"));
    }

    SECTION("rejects malformed segment name")
    {
        VideoSession video_session;
        video_session.add_session("s1", "v1");

        REQUIRE_FALSE(video_session.validate_segment_access("s1", "v1", "bad-segment"));
    }

    SECTION("bans suspicious start when too many prefetch segments")
    {
        VideoSession video_session;
        video_session.add_session("s1", "v1");

        REQUIRE(video_session.validate_segment_access("s1", "v1", "video_000.ts"));
        REQUIRE(video_session.validate_segment_access("s1", "v1", "video_001.ts"));
        REQUIRE(video_session.validate_segment_access("s1", "v1", "video_002.ts"));
        REQUIRE(video_session.validate_segment_access("s1", "v1", "video_003.ts"));
        REQUIRE(video_session.validate_segment_access("s1", "v1", "video_004.ts"));

        REQUIRE_FALSE(video_session.validate_segment_access("s1", "v1", "video_005.ts"));
    }

    SECTION("bans repeated large forward jumps")
    {
        VideoSession video_session;
        video_session.add_session("s1", "v1");
        video_session.start_session("s1", "v1");

        REQUIRE(video_session.validate_segment_access("s1", "v1", "video_000.ts"));
        REQUIRE(video_session.validate_segment_access("s1", "v1", "video_010.ts"));
        REQUIRE(video_session.validate_segment_access("s1", "v1", "video_020.ts"));

        REQUIRE_FALSE(video_session.validate_segment_access("s1", "v1", "video_030.ts"));
    }

    SECTION("bans repeated large backward jumps")
    {
        VideoSession video_session;
        video_session.add_session("s1", "v1");
        video_session.start_session("s1", "v1");

        REQUIRE(video_session.validate_segment_access("s1", "v1", "video_010.ts"));
        REQUIRE(video_session.validate_segment_access("s1", "v1", "video_007.ts"));
        REQUIRE(video_session.validate_segment_access("s1", "v1", "video_004.ts"));

        REQUIRE_FALSE(video_session.validate_segment_access("s1", "v1", "video_001.ts"));
    }
}

TEST_CASE("VideoSession::reset_session")
{
    SECTION("resets segment history and allows playback from beginning")
    {
        VideoSession video_session;
        video_session.add_session("s1", "v1");
        video_session.start_session("s1", "v1");

        REQUIRE(video_session.validate_segment_access("s1", "v1", "video_000.ts"));
        REQUIRE(video_session.validate_segment_access("s1", "v1", "video_010.ts"));

        video_session.reset_session("s1", "v1");

        REQUIRE(video_session.validate_segment_access("s1", "v1", "video_000.ts"));
    }
}
