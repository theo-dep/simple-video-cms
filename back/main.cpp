#include "server.h"

#include "filesystem.h"
#include "logging.h"

int main(int argc, const char** argv)
{
    filesystem::set_current_path({ argv, static_cast<std::size_t>(argc) });
    logging::init(filesystem::logs_path() / "back.log");
    return server::start();
}
