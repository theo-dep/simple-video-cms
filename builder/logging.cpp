#include "logging.h"

#include "filesystem.h"
#include "stringutils.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>

namespace logging
{
    // only the name, without return type and parameters
    std::string light_function_name(const std::source_location& location) noexcept;

    // two times a day, test if the file is more than 500 Mo
    constexpr std::int64_t hour_to_check{ 12 };
    constexpr std::uintmax_t bytes_to_check{ 500L * 1024L * 1024L };
    // then, open another log file in the limit of 5
    constexpr std::size_t max_log_file_id{ 5 };

    class Logger
    {
    public:
        Logger() noexcept;
        ~Logger() noexcept;

        void open(const std::filesystem::path& log_file_path) noexcept;

        template <typename T>
        Logger& operator<<(const T& message) noexcept;

    protected:
        std::filesystem::path log_id_file_path() const noexcept;
        void flush() noexcept;
        bool is_max_log_reached() noexcept;

    private:
        std::filesystem::path _log_file_path;
        std::size_t _log_file_id{ 0 };
        std::filesystem::file_time_type _latest_time_check;

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
        [&]() noexcept {
            while (_is_running) {
                flush();

                if (is_max_log_reached()) {
                    _log_file.close();
                    open(_log_file_path);
                }

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
    const std::filesystem::path log_file_dir{ log_file_path.parent_path() };
    if (!filesystem::create(log_file_dir)) {
        return;
    }

    _log_file_path = log_file_path;
    ++_log_file_id;

    if (_log_file_id > max_log_file_id)
        _log_file_id = 1;

    _log_file.open(log_id_file_path());
    _latest_time_check = std::filesystem::file_time_type::clock::now();

    logging::info{ R"(Log open at "{}")", log_id_file_path().c_str() };
}

template <typename T>
inline logging::Logger& logging::Logger::operator<<(const T& message) noexcept
{
    _log_file << message; // NOLINT(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
    std::clog << message; // NOLINT(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
    return *this;
}

inline std::filesystem::path logging::Logger::log_id_file_path() const noexcept
{
    return _log_file_path.string() + '.' + su::int_to_string(static_cast<int>(_log_file_id));
}

inline void logging::Logger::flush() noexcept
{
    if (_log_file.is_open()) {
        _log_file.flush();
    }

    std::clog.flush();
}

inline bool logging::Logger::is_max_log_reached() noexcept
{
    if (!_log_file.is_open())
        return false;

    std::error_code error_code;
    const std::filesystem::file_time_type last_log_written{ std::filesystem::last_write_time(log_id_file_path(), error_code) };
    if (error_code) {
        logging::error{ R"(Fail to get the log last write time of "{}": {})", log_id_file_path().string(), error_code.message() };
        return false;
    }

    const std::chrono::duration elapsed{ last_log_written - _latest_time_check };
    if (std::chrono::duration_cast<std::chrono::hours>(elapsed).count() >= hour_to_check) {
        _latest_time_check = last_log_written;

        const std::uintmax_t log_bytes_size{ std::filesystem::file_size(log_id_file_path(), error_code) };
        if (error_code) {
            logging::error{ R"(Fail to get the log size of "{}": {} ({}))", log_id_file_path().string(), error_code.message(), error_code.value() };
            return false;
        }

        return (log_bytes_size >= bytes_to_check);
    }

    return false;
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
