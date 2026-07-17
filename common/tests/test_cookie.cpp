#include <cookie.h>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("cookie::insert_to_cookie")
{
    SECTION("builds expected security attributes")
    {
        const auto header = cookie::insert_to_cookie("id", "deadbeef", std::chrono::seconds{ 120 });

        REQUIRE(header.find("id=deadbeef") != std::string::npos);
        REQUIRE(header.find("HttpOnly") != std::string::npos);
        REQUIRE(header.find("Path=/") != std::string::npos);
        REQUIRE(header.find("SameSite=Strict") != std::string::npos);
        REQUIRE(header.find("Max-Age=120") != std::string::npos);

#ifndef _DEBUG
        REQUIRE(header.find("__Host-id=deadbeef") != std::string::npos);
        REQUIRE(header.find("Secure") != std::string::npos);
#else
        REQUIRE(header.find("__Host-id=deadbeef") == std::string::npos);
#endif
    }
}

TEST_CASE("cookie::value_from_cookie")
{
    SECTION("extracts expected value")
    {
#ifndef _DEBUG
        const std::string cookie_header{ "theme=light; __Host-id=deadbeef; token=abc" };
#else
        const std::string cookie_header{ "theme=light; id=deadbeef; token=abc" };
#endif
        const auto value = cookie::value_from_cookie(cookie_header, "id");

        REQUIRE(value == "deadbeef");
    }

    SECTION("returns empty for missing or invalid values")
    {
        REQUIRE(cookie::value_from_cookie("id=NotHex", "id").empty());
        REQUIRE(cookie::value_from_cookie("theme=light; token=beef", "id").empty());
    }
}
