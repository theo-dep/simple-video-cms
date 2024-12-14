#include "server.h"

#include "logging.h"

int main()
{
    logging::init(std::filesystem::current_path() / "data" / "logs" / "back.log");
    return server::start();
}
