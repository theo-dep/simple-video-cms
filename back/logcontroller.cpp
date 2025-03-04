#include "logcontroller.h"

#include "logging.h"

namespace logcontroller
{
    std::pair<std::size_t, std::size_t> log_date_position(const std::string& log);
    std::string remove_log_date(const std::string& log);
    std::string add_log_date(const std::string& log);
}

LogController::LogController(const std::string& pattern)
    : _pattern{ pattern }
{
}

bool LogController::append(const std::string& log)
{
    const std::lock_guard<std::mutex> lock(_mutex);

    if (log.contains(_pattern)) {
        const std::string no_date_log{ logcontroller::remove_log_date(log) };
        if (!_buffer.empty() && _buffer[0] != no_date_log) {
            // flush logs because it is a new
            flush_no_lock();
        }

        // retain log
        _buffer.push_back(no_date_log);
        return true;
    }

    return false;
}

void LogController::flush()
{
    const std::lock_guard<std::mutex> lock(_mutex);

    if (_buffer.empty()) {
        return;
    }

    flush_no_lock();
}

void LogController::flush_no_lock()
{
    logging::raw_log(std::format("{} x {}", logcontroller::add_log_date(_buffer[0]), _buffer.size()));
    _buffer.clear();
}

inline std::pair<std::size_t, std::size_t> logcontroller::log_date_position(const std::string& log)
{
    const std::size_t date_start{ log.find('[') };
    const std::size_t date_end{ log.find(']') };
    return { date_start, date_end };
}

inline std::string logcontroller::remove_log_date(const std::string& log)
{
    // find indices to extract the date from log
    const auto [date_start, date_end]{ log_date_position(log) };

    if (date_start == std::string::npos || date_end == std::string::npos) {
        return log; // unknown format
    }

    // remove date from log
    return log.substr(0, date_start + 1) + log.substr(date_end);
}

inline std::string logcontroller::add_log_date(const std::string& log)
{
    // find indices to add the date from log
    const auto [date_start, date_end]{ log_date_position(log) };

    if (date_start == std::string::npos || date_end == std::string::npos) {
        return log; // unknown format
    }

    // add date from log
    return log.substr(0, date_start + 1) + logging::time_local() + log.substr(date_end);
}
