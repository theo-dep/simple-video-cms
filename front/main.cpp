#include "server.h"

#include "logging.h"

int main()
{
    logging::init(std::filesystem::current_path() / "data" / "logs" / "front.log");
    return server::start();
}
