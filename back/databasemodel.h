#pragma once

#include <optional>
#include <string>

struct User
{
    int id{ 0 };
    std::string name;
    std::optional<std::string> password{ std::nullopt };
    std::string salt;
};

struct SuperAdmin
{
    int id{ 0 };
};

struct Admin
{
    int id{ 0 };
};

struct Group
{
    int id{ 0 };
    std::string name;
};

struct Video
{
    int id{ 0 };
    std::string title;
};

struct GroupUser
{
    int group_id{ 0 };
    int user_id{ 0 };
};

struct VideoUserRight
{
    int video_id{ 0 };
    int user_id{ 0 };
};

struct VideoGroupRight
{
    int video_id{ 0 };
    int group_id{ 0 };
};
