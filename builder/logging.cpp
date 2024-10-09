#include "logging.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>

namespace logging
{
    // only the name, without return type and parameters
    std::string light_function_name(const std::source_location& location) noexcept;

    class Logger
    {
    public:
        Logger() noexcept;
        ~Logger() noexcept;

        void open(const std::filesystem::path& log_file_path) noexcept;

        template <typename T>
        Logger& operator<<(const T& message) noexcept;

    protected:
        void flush() noexcept;

    private:
        std::ofstream _log_file;
        bool _is_running{ true };
        std::thread _flush_thread;

    public:
        // prevent copy/move
        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;
        Logger(Logger&&) = delete;
        Logger& operator=(Logger&&) = delete;
    };

    Logger& logger() noexcept;
    void log(const std::string& type, const std::source_location& location, const std::string& message) noexcept;
}

void logging::init(const std::filesystem::path& log_file_path) noexcept
{
    logger().open(log_file_path);
}

std::string logging::time_local() noexcept
{
    const std::chrono::time_point p{ std::chrono::system_clock::now() };
    const std::time_t t{ std::chrono::system_clock::to_time_t(p) };

    std::stringstream ss;
    ss << std::put_time(std::localtime(&t), "%d/%b/%Y:%H:%M:%S %z");
    return ss.str();
}

void logging::raw_log(const std::string& message) noexcept
{
    logger() << message << '\n';
}

logging::info<std::string>::info(const std::string& message, const std::source_location& location) noexcept
{
    log("MSG", location, message);
}

logging::error<std::string>::error(const std::string& message, const std::source_location& location) noexcept
{
    log("ERR", location, message);
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

inline logging::Logger::Logger() noexcept
    : _flush_thread{
        [&]() {
            while (_is_running) {
                flush();

                using namespace std::chrono_literals;
                std::this_thread::sleep_for(3s);
            }
        }
    }
{
}

inline logging::Logger::~Logger() noexcept
{
    if (_flush_thread.joinable()) {
        _is_running = false;
        _flush_thread.join();
    }

    flush(); // flush one more time for precaution
}

inline void logging::Logger::open(const std::filesystem::path& log_file_path) noexcept
{
    _log_file.open(log_file_path);
}

template <typename T>
inline logging::Logger& logging::Logger::operator<<(const T& message) noexcept
{
    _log_file << message; // NOLINT(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
    std::clog << message; // NOLINT(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
    return *this;
}

inline void logging::Logger::flush() noexcept
{
    if (_log_file.is_open()) {
        _log_file.flush();
    }

    std::clog.flush();
}

inline logging::Logger& logging::logger() noexcept
{
    static Logger instance;
    return instance;
}

inline void logging::log(const std::string& type, const std::source_location& location, const std::string& message) noexcept
{
    logger() << type << " - " << time_local() << " - " << message << " - " << light_function_name(location)
             << " (" << location.file_name() << " at line " << location.line() << ")" << '\n';
}
