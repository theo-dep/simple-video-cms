#include "server.h"

#include "logging.h"

int main()
{
    logging::init(std::filesystem::current_path() / "front.log");
    return server::start();
}
