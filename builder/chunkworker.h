#pragma once

#include <functional>
#include <future>
// #include <mutex>
#include <optional>
#include <string>

class ChunkWorker
{
public:
    ChunkWorker() noexcept = default;
    ~ChunkWorker() noexcept;

    void set_buffer_size(std::size_t size) noexcept;

    void set_fetch_async_callback(const std::function<bool()>& callback) noexcept;

    std::function<bool(const char*, std::size_t)> append_chunk_callback() noexcept;

    void start_fetch_async() noexcept;

    bool fetch_result() noexcept;

    void start_chunk_at(const std::string& range_header) noexcept;
    std::size_t chunk_offset() const { return _offset; }

    void wait_for_chunk() const noexcept;
    std::string chunk() noexcept;

private:
    std::size_t min_chunk_size() const noexcept;
    bool is_chunk_ready() const noexcept;

    std::function<bool()> _fetch_callback;
    std::future<bool> _fetch_future;
    bool _request_interruption;
    std::optional<bool> _fetch_result{ std::nullopt };

    std::size_t _offset{ 0 };
    std::size_t _length{ 0 };
    std::string _buffer;
    // mutable std::mutex _mutex;

    // prevent copy/move
    ChunkWorker(const ChunkWorker&) = delete;
    ChunkWorker& operator=(const ChunkWorker&) = delete;
    ChunkWorker(ChunkWorker&&) = delete;
    ChunkWorker& operator=(ChunkWorker&&) = delete;
};
