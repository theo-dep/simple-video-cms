#include "server.h"

#include "filesystem.h"
#include "logging.h"

int main()
{
    logging::init(filesystem::logs_path() / "back.log");
    return server::start();
}
