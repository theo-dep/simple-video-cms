#pragma once

#include "databasemodel.h"

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_ONLY_SERIALIZE(Video, id, title)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_ONLY_SERIALIZE(Group, id, name)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_ONLY_SERIALIZE(User, id, name)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_ONLY_SERIALIZE(Author, id, name)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_ONLY_SERIALIZE(Location, id, name)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_ONLY_SERIALIZE(Tag, id, name)

struct VideoInfo
{
    int id{ 0 };
    std::string title;
    bool bookmarked{ false };
    std::optional<std::string> date;
    std::optional<std::string> location;
    std::vector<std::string> authors;
    std::vector<std::string> tags;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_ONLY_SERIALIZE(VideoInfo, id, title, bookmarked, date, location, authors, tags)

struct ConnectedUser
{
    int id{ 0 };
    std::string name;
    bool is_admin{ false };
    bool is_first_connection{ false };
    std::vector<VideoInfo> videos;
};

template <typename BasicJsonType>
void to_json(BasicJsonType& json, const ConnectedUser& user)
{
    json["id"] = user.id;
    json["name"] = user.name;
    json["isAdmin"] = user.is_admin;
    json["isFirstConnection"] = user.is_first_connection;
    json["videos"] = user.videos;
}

struct AdminUserInfo
{
    int id{ 0 };
    std::string name;
    std::vector<Group> groups;
    std::vector<Video> videos;
    bool is_logged_once{ false };
    bool is_deactivated{ false };
};

template <typename BasicJsonType>
void to_json(BasicJsonType& json, const AdminUserInfo& user)
{
    json["id"] = user.id;
    json["name"] = user.name;
    json["groups"] = user.groups;
    json["videos"] = user.videos;
    json["isLoggedOnce"] = user.is_logged_once;
    json["isDeactivated"] = user.is_deactivated;
}

struct AdminAdminInfo
{
    int id{ 0 };
    std::string name;
    bool is_super_admin{ false };
    bool is_logged_once{ false };
    bool is_deactivated{ false };
};

template <typename BasicJsonType>
void to_json(BasicJsonType& json, const AdminAdminInfo& user)
{
    json["id"] = user.id;
    json["name"] = user.name;
    json["isSuperAdmin"] = user.is_super_admin;
    json["isLoggedOnce"] = user.is_logged_once;
    json["isDeactivated"] = user.is_deactivated;
}

struct AdminGroupInfo
{
    int id{ 0 };
    std::string name;
    std::vector<User> users;
    std::vector<Video> videos;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_ONLY_SERIALIZE(AdminGroupInfo, id, name, users, videos)

struct AdminVideoInfo
{
    int id{ 0 };
    std::string title;
    std::optional<std::string> date;
    std::optional<Location> location;
    std::vector<Author> authors;
    std::vector<Tag> tags;
    std::vector<Group> groups;
    std::vector<User> users;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_ONLY_SERIALIZE(AdminVideoInfo, id, title, date, location, authors, tags, groups, users)
