#pragma once

#include <ormpp.hpp>

struct Admin
{
    int id;
    std::string username;
    std::string password;
};
REFLECTION(Admin, id, username, password)

struct User
{
    int id;
    std::string username;
    std::string password;
};
REFLECTION(User, id, username, password)

struct Video
{
    int id;
    std::string title;
    std::string uploader;
    int view_count;
    std::string upload_date;
};
REFLECTION(Video, id, title, uploader, view_count, upload_date)
