#include "logging.h"

#include <chrono>
#include <fstream>
#include <functional>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>

namespace logging
{
    // only the name, without return type and parameters
    std::string light_function_name(const std::source_location& location) noexcept;

    class Logger
    {
    public:
        explicit Logger(const std::filesystem::path& log_file_path) noexcept;
        ~Logger() noexcept;

        void log(std::ostream& stream, const std::string& message) noexcept;

    protected:
        void flush_locals(std::string& cout_str, std::string& cerr_str) noexcept;

    private:
        std::mutex _mutex;
        std::ofstream _log_file;
        bool _is_running;
        std::thread _flush_thread;

        std::ostringstream _cout_local;
        std::ostringstream _cerr_local;

        std::streambuf* const _cout_buffer;
        std::streambuf* const _cerr_buffer;
    };

    Logger& instance(const std::filesystem::path& log_file_path = {}) noexcept;
}

void logging::init(const std::filesystem::path& log_file_path) noexcept
{
    // create the instance
    instance(log_file_path);
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
    instance().log(std::cout, message);
}

logging::log<std::string>::log(std::ostream& stream, const std::source_location& location, const std::string& message) noexcept
{
    const std::string formated_message{
        std::format("{} - {} - {} ({} at line {})",
                    time_local(), message, light_function_name(location),
                    location.file_name(), location.line())
    };
    instance().log(stream, formated_message);
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

inline logging::Logger::Logger(const std::filesystem::path& log_file_path) noexcept
    : _log_file{ log_file_path, std::ios::out | std::ios::trunc }
    , _is_running{ true }
    , _flush_thread{
        [&]() {
            std::string cout_str, cerr_str;

            while (_is_running) {
                flush_locals(cout_str, cerr_str);

                using namespace std::chrono_literals;
                std::this_thread::sleep_for(3s);
            }
        }
    } // save to restore in destructor
    , _cout_buffer{ std::cout.rdbuf() }
    , _cerr_buffer{ std::cerr.rdbuf() }
{
    std::cout.rdbuf(_cout_local.rdbuf());
    std::cerr.rdbuf(_cerr_local.rdbuf());
}

inline logging::Logger::~Logger() noexcept
{
    if (_flush_thread.joinable()) {
        _is_running = false;
        _flush_thread.join();
    }

    std::cout.rdbuf(_cout_buffer);
    std::cerr.rdbuf(_cerr_buffer);
}

inline void logging::Logger::log(std::ostream& stream, const std::string& message) noexcept
{
    const std::lock_guard<std::mutex> lock(_mutex);
    stream << message << '\n';
}

inline void logging::Logger::flush_locals(std::string& cout_str, std::string& cerr_str) noexcept
{
    const std::lock_guard<std::mutex> lock(_mutex);

    cout_str = _cout_local.str();
    cerr_str = _cerr_local.str();

    // clear
    _cout_local.str(std::string{});
    _cerr_local.str(std::string{});

    if (_log_file.is_open()) {
        if (!cout_str.empty()) {
            _log_file << "MSG: " << cout_str;
        }
        if (!cerr_str.empty()) {
            _log_file << "ERR: " << cerr_str;
        }
        _log_file.flush();
    }

    if (!cout_str.empty()) {
        std::cout.rdbuf(_cout_buffer);
        std::cout << cout_str << std::flush;
        std::cout.rdbuf(_cout_local.rdbuf());
    }
    if (!cerr_str.empty()) {
        std::cerr.rdbuf(_cerr_buffer);
        std::cerr << cerr_str << std::flush;
        std::cerr.rdbuf(_cerr_local.rdbuf());
    }
}

inline logging::Logger& logging::instance(const std::filesystem::path& log_file_path) noexcept
{
    static std::once_flag flag;
    static std::unique_ptr<Logger> instance;
    std::call_once(flag, [&]() {
        if (!instance) {
            instance = std::make_unique<Logger>(log_file_path);
        }
    });
    return *instance;
}
