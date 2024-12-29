#include "chunkworker.h"

#include "logging.h"
#include "stringutils.h"

#include <chrono>
#include <cstring>
#include <regex>

using namespace std::chrono_literals;

ChunkWorker::~ChunkWorker() noexcept
{
    if (!_fetch_result.has_value()) {
        _request_interruption = true;
        _fetch_future.wait_for(1s);
    }
}

std::size_t ChunkWorker::buffer_size() const
{
    // const std::lock_guard<std::mutex> lock(_mutex);
    return _buffer.size();
}

void ChunkWorker::set_fetch_async_callback(const std::function<bool()>& callback) noexcept
{
    _fetch_callback = callback;
}

std::function<bool(const char*, std::size_t)> ChunkWorker::append_chunk_callback() noexcept
{
    return [this](const char* data, std::size_t size) noexcept -> bool {
        // const std::lock_guard<std::mutex> lock(_mutex);
        const std::size_t append_size{ std::min(_buffer.size() - _filled_chunk_size, size) };
        std::memcpy(&_buffer[_filled_chunk_size], data, append_size);
        _filled_chunk_size += append_size;
        return _filled_chunk_size < _buffer.size() && !_request_interruption;
    };
}

void ChunkWorker::start_fetch_async() noexcept
{
    _request_interruption = false;
    _fetch_result.reset();
    _fetch_future = std::async(std::launch::async, _fetch_callback);
}

bool ChunkWorker::fetch_result() noexcept
{
    try {
        if (_fetch_result.has_value())
            return _fetch_result.value();

        if (!_fetch_future.valid())
            return false;

        if (_fetch_future.wait_for(1ms) != std::future_status::ready) // work in progress
            return true;

        // buffer is full or there is an error
        _fetch_result.emplace(_filled_chunk_size == _buffer.size() || _fetch_future.get());
        return _fetch_result.value();

    } catch (const std::exception& e) {
        logging::error{ "Fail to fetch result: {}", e.what() };
        return false;
    }
}

std::size_t ChunkWorker::start_chunk_at(const std::string& range_header, std::size_t max_size) noexcept
{
    static std::regex pattern;
    static std::once_flag create_pattern_once;
    std::call_once(create_pattern_once, [] noexcept {
        try {
            pattern = "bytes=(\\d+)-(\\d*)";
        } catch (const std::exception& e) {
            logging::error{ "Fail to create chunk regular expression: {}", e.what() };
        }
    });

    std::size_t offset{ 0 };
    std::smatch range_match;
    if (std::regex_match(range_header, range_match, pattern) && range_match.size() >= 3) {
        // index 0 is the whole pattern matched
        // 1 is the range start
        // 2 is the range end if any
        offset = su::string_to_int(range_match[1].str());
        std::size_t length{ max_size };

        const std::string length_str{ range_match[2].str() };
        if (!length_str.empty()) {
            length = su::string_to_int(length_str) + 1;
        }

        // const std::lock_guard<std::mutex> lock(_mutex);
        _buffer.resize(length - offset);
    }

    return offset;
}

std::string ChunkWorker::chunk() noexcept
{
    const std::size_t chunk_size{ min_chunk_size() };

    // const std::lock_guard<std::mutex> lock(_mutex);
    const std::string chunk(&_buffer[_sent_chunk_size], chunk_size);
    _sent_chunk_size += chunk_size;
    return chunk;
}

std::size_t ChunkWorker::min_chunk_size() const noexcept
{
    // const std::lock_guard<std::mutex> lock(_mutex);
    constexpr std::size_t chunk_size{ 102400 };
    return std::clamp<std::size_t>(_filled_chunk_size - _sent_chunk_size, 0, chunk_size);
}
