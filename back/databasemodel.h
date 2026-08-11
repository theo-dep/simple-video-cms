#pragma once

#include <optional>
#include <string>

struct User
{
    int id{ 0 };
    std::string name;
    std::optional<std::string> password{ std::nullopt };
    std::string salt;
    bool deactivated{ false };

    static constexpr auto table_name = "users";
};

struct SuperAdmin
{
    int id{ 0 };

    static constexpr auto table_name = "super_admins";
};

struct Admin
{
    int id{ 0 };

    static constexpr auto table_name = "admins";
};

struct Group
{
    int id{ 0 };
    std::string name;

    static constexpr auto table_name = "groups";
};

struct Video
{
    int id{ 0 };
    std::string title;
    std::optional<std::string> date{ std::nullopt };
    std::optional<int> place_id{ std::nullopt };

    static constexpr auto table_name = "videos";
};

struct GroupUser
{
    int group_id{ 0 };
    int user_id{ 0 };

    static constexpr auto table_name = "user_groups";
};

struct VideoUserRight
{
    int video_id{ 0 };
    int user_id{ 0 };

    static constexpr auto table_name = "video_user_rights";
};

struct VideoGroupRight
{
    int video_id{ 0 };
    int group_id{ 0 };

    static constexpr auto table_name = "video_group_rights";
};

struct UserVideoBookmark
{
    int user_id{ 0 };
    int video_id{ 0 };

    static constexpr auto table_name = "user_video_bookmarks";
};

struct Author
{
    int id{ 0 };
    std::string name;

    static constexpr auto table_name = "authors";
};

struct VideoAuthor
{
    int video_id{ 0 };
    int author_id{ 0 };

    static constexpr auto table_name = "video_authors";
};

struct Place
{
    int id{ 0 };
    std::string name;

    static constexpr auto table_name = "places";
};

struct Tag
{
    int id{ 0 };
    std::string name;

    static constexpr auto table_name = "tags";
};

struct VideoTag
{
    int video_id{ 0 };
    int tag_id{ 0 };

    static constexpr auto table_name = "video_tags";
};

struct SessionInfo
{
    std::string id;
    int user_id{ 0 };
    std::string creation_date;
    std::string max_age_time;

    static constexpr auto table_name = "sessions";
};
