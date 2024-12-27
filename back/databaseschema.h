#include <string>

// Structure for users
struct User
{
    int id{ 0 };
    std::string name;
    std::string password;
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

// Structure for videos
struct Video
{
    int id{ 0 };
    std::string title;
    int views{ 0 };
};

// Structure for video rights
struct VideoRight
{
    int video_id{ 0 };
    int user_id{ 0 };
};
