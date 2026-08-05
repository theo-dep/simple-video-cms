#include <stringutils.h>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("su::split")
{
    SECTION("returns tokens using provided delimiter")
    {
        const auto parts = su::split("alpha,beta,gamma", ',');

        REQUIRE(parts.size() == 3);
        REQUIRE(parts[0] == "alpha");
        REQUIRE(parts[1] == "beta");
        REQUIRE(parts[2] == "gamma");
    }
}

TEST_CASE("su::trim")
{
    SECTION("removes surrounding whitespace")
    {
        std::string value{ "  \t user name \n" };
        const auto val_trimmed = su::trim(value);

        REQUIRE(val_trimmed == "user name");
    }
}

TEST_CASE("su::lower")
{
    SECTION("converts ASCII letters to lowercase")
    {
        std::string value{ "AdMiN_42" };
        const auto val_lowered = su::lower(value);

        REQUIRE(val_lowered == "admin_42");
    }
}

TEST_CASE("su::string_to_bool")
{
    SECTION("parses true values")
    {
        REQUIRE(su::string_to_bool("True") == true);
        REQUIRE(su::string_to_bool("TRUE") == true);
        REQUIRE(su::string_to_bool("true") == true);
        REQUIRE(su::string_to_bool("1") == true);
    }

    SECTION("parses false values")
    {
        REQUIRE(su::string_to_bool("False") == false);
        REQUIRE(su::string_to_bool("FALSE") == false);
        REQUIRE(su::string_to_bool("false") == false);
        REQUIRE(su::string_to_bool("0") == false);

        REQUIRE(su::string_to_bool("42") == false);
        REQUIRE(su::string_to_bool("bad") == false);
        REQUIRE(su::string_to_bool("12bad") == false);
    }
}

TEST_CASE("su::string_to_int")
{
    SECTION("parses integer values")
    {
        REQUIRE(su::string_to_int("42") == 42);
        REQUIRE(su::string_to_int("-7") == -7);
    }

    SECTION("returns fallback value for invalid string")
    {
        REQUIRE(su::string_to_int("bad") == 0);
    }

    SECTION("accepts valid numeric prefix")
    {
        REQUIRE(su::string_to_int("12bad") == 12);
    }
}

TEST_CASE("su::int_to_string")
{
    SECTION("formats integer values")
    {
        REQUIRE(su::int_to_string(0) == "0");
        REQUIRE(su::int_to_string(2048) == "2048");
        REQUIRE(su::int_to_string(-12) == "-12");
    }
}

TEST_CASE("su::time_point_to_string")
{
    SECTION("formats time_point as ISO 8601 UTC")
    {
        const auto tp = std::chrono::sys_days{ std::chrono::year{ 2024 } / 3 / 15 } + std::chrono::hours{ 10 } + std::chrono::minutes{ 30 } + std::chrono::seconds{ 5 };

        REQUIRE(su::time_point_to_string(tp) == "2024-03-15T10:30:05Z");
    }
}

TEST_CASE("su::string_to_time_point")
{
    SECTION("parses ISO 8601 UTC string")
    {
        const auto tp = su::string_to_time_point("2024-03-15T10:30:05Z");
        const auto expected = std::chrono::sys_days{ std::chrono::year{ 2024 } / 3 / 15 } + std::chrono::hours{ 10 } + std::chrono::minutes{ 30 } + std::chrono::seconds{ 5 };

        REQUIRE(tp.has_value());
        REQUIRE(tp.value() == expected);
    }

    SECTION("returns nullopt for invalid string")
    {
        REQUIRE(su::string_to_time_point("bad") == std::nullopt);
        REQUIRE(su::string_to_time_point("2024-13-99T99:99:99Z") == std::nullopt);
    }
}

TEST_CASE("su::seconds_to_string")
{
    SECTION("formats seconds values")
    {
        REQUIRE(su::seconds_to_string(std::chrono::seconds{ 0 }) == "0");
        REQUIRE(su::seconds_to_string(std::chrono::seconds{ 3600 }) == "3600");
        REQUIRE(su::seconds_to_string(std::chrono::seconds{ -12 }) == "-12");
    }
}

TEST_CASE("su::string_to_seconds")
{
    SECTION("parses seconds values")
    {
        REQUIRE(su::string_to_seconds("42") == std::chrono::seconds{ 42 });
        REQUIRE(su::string_to_seconds("-7") == std::chrono::seconds{ -7 });
    }

    SECTION("returns fallback value for invalid string")
    {
        REQUIRE(su::string_to_seconds("bad") == std::chrono::seconds{ 0 });
    }
}
