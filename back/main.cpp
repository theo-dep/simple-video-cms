#include "database.h"
#include "logging.h"
#include "server.h"

#include <thread>

int main()
{
    logging::init(std::filesystem::current_path() / "back.log");

    while (!database::is_open()) {
        logging::info{ "Wait for database..." };
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(10s);
    }

    return server::start();
}
