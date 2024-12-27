#include "server.h"

#include "filesystem.h"
#include "logging.h"

int main()
{
    logging::init(filesystem::logs_path() / "front.log");
    return server::start();
}
