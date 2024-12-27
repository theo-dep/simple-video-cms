#include "chunkworker.h"

#include "logging.h"
#include "stringutils.h"

#include <chrono>
#include <format>
#include <regex>

using namespace std::chrono_literals;

ChunkWorker::~ChunkWorker() noexcept
{
    if (!_fetch_result.has_value()) {
        _request_interruption = true;
        _fetch_future.wait_for(1s);
    }
}

void ChunkWorker::set_buffer_size(std::size_t size) noexcept
{
    // const std::lock_guard<std::mutex> lock(_mutex);
    _buffer.reserve(size);
    _offset = 0;
    _length = size;
}

void ChunkWorker::set_fetch_async_callback(const std::function<bool()>& callback) noexcept
{
    _fetch_callback = callback;
}

std::function<bool(const char*, std::size_t)> ChunkWorker::append_chunk_callback() noexcept
{
    return [this](const char* data, std::size_t size) noexcept -> bool {
        // const std::lock_guard<std::mutex> lock(_mutex);
        _buffer.append(data, size);
        return !_request_interruption;
    };
}

void ChunkWorker::start_fetch_async() noexcept
{
    _request_interruption = false;
    _fetch_future = std::async(std::launch::async, _fetch_callback);
    _fetch_result.reset();
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

        _fetch_result.emplace(_fetch_future.get());
        return _fetch_result.value();

    } catch (const std::exception& e) {
        logging::error{ "Fail to fetch result: {}", e.what() };
        return false;
    }
}

void ChunkWorker::start_chunk_at(const std::string& range_header) noexcept
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

    std::smatch range_match;
    if (std::regex_match(range_header, range_match, pattern) && range_match.size() >= 3) {
        // index 0 is the whole pattern matched
        // 1 is the range start
        // 2 is the range end if any
        _offset = su::string_to_int(range_match[1].str());
        _buffer.insert(0, _offset, '0');

        const std::string length_str{ range_match[2].str() };
        _length = length_str.empty() ? _buffer.capacity() - _offset : su::string_to_int(length_str);
    }
}

void ChunkWorker::wait_for_chunk() const noexcept
{
    while (!is_chunk_ready())
        std::this_thread::sleep_for(10ms);
}

std::string ChunkWorker::chunk() noexcept
{
    if (!is_chunk_ready())
        return {};

    // const std::lock_guard<std::mutex> lock(_mutex);
    const std::size_t chunk_size{ min_chunk_size() };
    const std::string chunk{ _buffer.substr(_offset, chunk_size) };
    _offset += chunk_size;
    _length -= chunk_size;
    return chunk;
}

std::size_t ChunkWorker::min_chunk_size() const noexcept
{
    constexpr std::size_t chunk_size{ 102400 };
    return std::min(_length, chunk_size);
}

bool ChunkWorker::is_chunk_ready() const noexcept
{
    // const std::lock_guard<std::mutex> lock(_mutex);
    return _offset < _buffer.size() && _buffer.size() >= (_offset + min_chunk_size());
}
