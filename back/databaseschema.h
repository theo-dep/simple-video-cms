#pragma once

#include "types.h"

#include <ormpp.hpp>

struct Admin
{
    int id;
    std::string username;
    std::string password;
};
REGISTER_AUTO_KEY(Admin, id);
REFLECTION(Admin, id, username, password)

struct User
{
    int id;
    std::string username;
    std::string password;
};
REGISTER_AUTO_KEY(User, id);
REFLECTION(User, id, username, password)

struct Video
{
    types::md5_varchar id;
    std::string title;
    std::string file_path;
    int view_count{ 0 };
};
REGISTER_CONFLICT_KEY(Video, id);
REFLECTION(Video, id, title, file_path, view_count)
