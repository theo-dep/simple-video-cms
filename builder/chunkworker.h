#pragma once

#include <functional>
#include <future>
// #include <mutex>
#include <optional>
#include <string>
#include <vector>

class ChunkWorker
{
public:
    ChunkWorker() noexcept = default;
    ~ChunkWorker() noexcept;

    std::size_t buffer_size() const;

    void set_fetch_async_callback(const std::function<bool()>& callback) noexcept;

    std::function<bool(const char*, std::size_t)> append_chunk_callback() noexcept;

    void start_fetch_async() noexcept;

    bool fetch_result() noexcept;

    std::size_t start_chunk_at(const std::string& range_header, std::size_t max_size) noexcept;

    std::string chunk() noexcept;

private:
    std::size_t min_chunk_size() const noexcept;

    std::function<bool()> _fetch_callback;
    std::future<bool> _fetch_future;
    bool _request_interruption;
    std::optional<bool> _fetch_result{ std::nullopt };

    std::size_t _sent_chunk_size{ 0 };
    std::size_t _filled_chunk_size{ 0 };
    std::vector<std::string::value_type> _buffer;
    // mutable std::mutex _mutex;

    // prevent copy/move
    ChunkWorker(const ChunkWorker&) = delete;
    ChunkWorker& operator=(const ChunkWorker&) = delete;
    ChunkWorker(ChunkWorker&&) = delete;
    ChunkWorker& operator=(ChunkWorker&&) = delete;
};
