#include "database.h"
#include "server.h"
#include "servercommon.h"

#include <thread>

int main()
{
    while (!database::is_open()) {
        MSG("Wait for database...");
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(10s);
    }

    return server::start();
}
