#include "server.h"

#include "logging.h"

int main()
{
    logging::init(std::filesystem::current_path() / "back.log");
    return server::start();
}
