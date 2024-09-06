#include <httplib.hpp>

int main()
{
    httplib::Server server;
    server.listen("0.0.0.0", 5000);
}
