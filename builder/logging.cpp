#include "logging.h"

#include <chrono>

std::string logging::time_local() noexcept
{
    const std::chrono::time_point p{ std::chrono::system_clock::now() };
    const std::time_t t{ std::chrono::system_clock::to_time_t(p) };

    std::stringstream ss;
    ss << std::put_time(std::localtime(&t), "%d/%b/%Y:%H:%M:%S %z");
    return ss.str();
}

logging::log<std::string>::log(std::ostream& stream, const std::source_location& location, const std::string& message) noexcept
{
    stream << time_local() << " - " << message << " - " << location.function_name()
           << " (" << location.file_name() << " at line " << location.line() << ")" << std::endl;
}

logging::info<std::string>::info(const std::string& message, const std::source_location& location) noexcept
{
    log<std::string>{ std::cout, location, message };
}

logging::error<std::string>::error(const std::string& message, const std::source_location& location) noexcept
{
    log<std::string>{ std::cerr, location, message };
}

logging::debug<std::string>::debug(const std::string& message, const std::source_location& location) noexcept
{
#ifdef DEBUG_LOG
    info<std::string>{ message, location };
#else
    (void)message;
    (void)location;
#endif
}
