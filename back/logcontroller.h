#pragma once

#include <mutex>
#include <string>
#include <vector>

class LogController
{
public:
    LogController(std::string pattern);

    bool append(const std::string& log);

    void flush();

protected:
    void flush_no_lock();

private:
    const std::string _pattern;
    std::vector<std::string> _buffer;
    mutable std::mutex _mutex;
};
