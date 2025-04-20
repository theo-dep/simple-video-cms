#include <optional>
#include <string>

// Structure for users
struct User
{
    int id{ 0 };
    std::string name;
    std::optional<std::string> password{ std::nullopt };
    std::string salt;
};

// Structure for super admins
struct SuperAdmin
{
    int id{ 0 };
};

// Structure for admins
struct Admin
{
    int id{ 0 };
};

// Struct for groups
struct Group
{
    int id{ 0 };
    std::string name;
};

// Structure for videos
struct Video
{
    int id{ 0 };
    std::string title;
    int views{ 0 };
};

// Struct for group users
struct GroupUser
{
    int group_id{ 0 };
    int user_id{ 0 };
};

// Structure for user video rights
struct VideoUserRight
{
    int video_id{ 0 };
    int user_id{ 0 };
};

// Structure for group video rights
struct VideoGroupRight
{
    int video_id{ 0 };
    int group_id{ 0 };
};
