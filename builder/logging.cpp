#include "logging.h"

#include <chrono>

namespace logging
{
    // only the name, without return type and parameters
    std::string light_function_name(const std::source_location& location) noexcept;
}

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
    stream << time_local() << " - " << message << " - " << light_function_name(location)
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

inline std::string logging::light_function_name(const std::source_location& location) noexcept
{
    const std::string function_name{ location.function_name() };
    const std::size_t start_index{ function_name.find_first_of(' ') + 1 };
    const std::size_t end_index{ function_name.find_first_of('(') };
    return std::string{ function_name, start_index, end_index - start_index };
}
