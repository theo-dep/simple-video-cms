#include "video.h"

#include "logging.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/mem.h>
#include <libavutil/opt.h>

#include <png.h>
}

#include <algorithm>
#include <cstring>
#include <memory>
#include <ranges>
#include <span>
#include <utility>
#include <vector>

namespace video
{
    struct AVDeleter
    {
        void operator()(AVFormatContext* format_context) const;
        void operator()(AVCodecContext* codex_context) const;
        void operator()(AVFrame* frame) const;
        void operator()(AVIOContext* avio_context) const;
        void operator()(AVFilterGraph* filter_graph) const;
        void operator()(AVFilterContext* filter_context) const;
        void operator()(AVPacket* packet) const;
    };

    using AVFormatContextPtr = std::unique_ptr<AVFormatContext, AVDeleter>;
    using AVCodecContextPtr = std::unique_ptr<AVCodecContext, AVDeleter>;
    using AVFramePtr = std::unique_ptr<AVFrame, AVDeleter>;
    using AVIOContextPtr = std::unique_ptr<AVIOContext, AVDeleter>;
    using AVFilterGraphPtr = std::unique_ptr<AVFilterGraph, AVDeleter>;
    using AVFilterContextPtr = std::unique_ptr<AVFilterContext, AVDeleter>;
    using AVPacketPtr = std::unique_ptr<AVPacket, AVDeleter>;

    const char* err2str(int error_numero);

    struct BufferData
    {
        std::span<char const> pointer;
        const std::span<char const> ref_pointer; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
    };
    int read_packet(void* opaque, std::uint8_t* buffer, int buffer_size);
    std::int64_t seek(void* opaque, std::int64_t offset, int whence);

    std::string frame_to_png(const AVFrame* frame);
    void write_data(png_structp png_ptr, png_bytep data, png_size_t length);
}

std::string video::thumbnail(const std::string& video_content, const std::string& format,
                             std::size_t width, std::size_t height, std::size_t n_images)
{
#ifndef NDEBUG
    av_log_set_level(AV_LOG_DEBUG);
#endif

    AVFormatContextPtr format_context(avformat_alloc_context());
    if (format_context == nullptr) {
        logging::error{ "Could not allocate format context" };
        return {};
    }

    // fill opaque structure used by the AVIOContext read callback
    BufferData buffer_data{
        .pointer = video_content,
        .ref_pointer = video_content
    };

    constexpr std::size_t avio_context_buffer_size{ 4096 };
    std::uint8_t* const avio_context_buffer{ static_cast<std::uint8_t*>(av_malloc(avio_context_buffer_size)) };
    if (avio_context_buffer == nullptr) {
        logging::error{ "Could not allocate avio context buffer" };
        return {};
    }

    const AVIOContextPtr avio_context(avio_alloc_context(avio_context_buffer, avio_context_buffer_size, 0, &buffer_data, &read_packet, nullptr, &seek));
    if (avio_context == nullptr) {
        logging::error{ "Could not allocate avio context" };
        return {};
    }

    format_context->pb = avio_context.get();
    format_context->flags = AVFMT_FLAG_CUSTOM_IO;
    format_context->iformat = av_find_input_format(format.c_str()); // not necessary
    constexpr std::int64_t probe_size{ 1200000 };
    format_context->probesize = probe_size;

    // Open the input video
    if (const int ret{ avformat_open_input(std::inout_ptr(format_context), nullptr, nullptr, nullptr) }; ret < 0) {
        logging::error{ "Could not open input: {}", err2str(ret) };
        return {};
    }

    logging::debug{ "Format: {}, Duration: {} us, Bitrate: {} bits/s", format_context->iformat->name, format_context->duration, format_context->bit_rate };

    if (const int ret{ avformat_find_stream_info(format_context.get(), nullptr) }; ret < 0) {
        logging::error{ "Could not find stream information: {}", err2str(ret) };
        return {};
    }

#ifdef DEBUG_LOG
    // Dump information about file onto standard error
    av_dump_format(format_context.get(), 0, nullptr, 0);
#endif

    if (format_context->pb != nullptr && format_context->pb->error != 0) {
        logging::error{ "Error reading video content: {}", err2str(format_context->pb->error) };
        return {};
    }

    const AVCodec* input_codec{ nullptr };
    int video_stream_index{ av_find_best_stream(format_context.get(), AVMEDIA_TYPE_VIDEO, -1, -1, &input_codec, 0) };
    if (video_stream_index < 0) {
        logging::error{ "Could not find video stream" };
        return {};
    }

    const AVCodecParameters* const input_codec_parameters{ format_context->streams[video_stream_index]->codecpar };

    const AVCodecContextPtr codec_context(avcodec_alloc_context3(input_codec));
    if (codec_context == nullptr) {
        logging::error{ "Could not allocate codec context" };
        return {};
    }

    if (const int ret{ avcodec_parameters_to_context(codec_context.get(), input_codec_parameters) }; ret < 0) {
        logging::error{ "Could not set codec parameters: {}", err2str(ret) };
        return {};
    }
    if (const int ret{ avcodec_open2(codec_context.get(), input_codec, nullptr) }; ret < 0) {
        logging::error{ "Could not open codec: {}", err2str(ret) };
        return {};
    }

    const AVFilterGraphPtr filter_graph{ avfilter_graph_alloc() };
    if (filter_graph == nullptr) {
        logging::error{ "Could not allocate filter graph" };
        return {};
    }

    const AVFilter* const buffer_source{ avfilter_get_by_name("buffer") };
    if (buffer_source == nullptr) {
        logging::error{ "Could not find buffer source filter" };
        return {};
    }

    const std::string filter_args{ std::format(
        "video_size={}x{}:pix_fmt={}:time_base={}/{}:pixel_aspect={}/{}",
        codec_context->width, codec_context->height, std::to_underlying(codec_context->pix_fmt),
        format_context->streams[video_stream_index]->time_base.num, format_context->streams[video_stream_index]->time_base.den,
        codec_context->sample_aspect_ratio.num, codec_context->sample_aspect_ratio.den) };

    AVFilterContextPtr buffer_source_context{ nullptr };
    if (const int ret{ avfilter_graph_create_filter(std::out_ptr(buffer_source_context), buffer_source, "in", filter_args.c_str(), nullptr, filter_graph.get()) }; ret < 0) {
        logging::error{ "Cannot create buffer source: {}", err2str(ret) };
        return {};
    }

    // Sink buffer
    const AVFilter* const buffer_sink{ avfilter_get_by_name("buffersink") };
    if (buffer_sink == nullptr) {
        logging::error{ "Could not find buffer sink filter" };
        return {};
    }

    AVFilterContextPtr buffer_sink_context{ nullptr };
    if (const int ret{ avfilter_graph_create_filter(std::out_ptr(buffer_sink_context), buffer_sink, "out", nullptr, nullptr, filter_graph.get()) }; ret < 0) {
        logging::error{ "Cannot create buffer sink: {}", err2str(ret) };
        return {};
    }

    constexpr std::array pixel_formats{ AV_PIX_FMT_RGB24, AV_PIX_FMT_NONE };
    if (const int ret{ av_opt_set_int_list(buffer_sink_context.get(), "pix_fmts", pixel_formats.data(), AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN) }; ret < 0) {
        logging::error{ "Cannot set output pixel format: {}", err2str(ret) };
        return {};
    }

    // thumbnail filter
    const AVFilter* thumbnail_filter{ avfilter_get_by_name("thumbnail") };
    if (thumbnail_filter == nullptr) {
        logging::error{ "Could not find thumbnail filter" };
        return {};
    }

    const std::string thumbnail_args{ std::format("n={}", n_images) };

    AVFilterContextPtr thumbnail_context{ nullptr };
    if (const int ret{ avfilter_graph_create_filter(std::out_ptr(thumbnail_context), thumbnail_filter, "thumbnail", thumbnail_args.c_str(), nullptr, filter_graph.get()) }; ret < 0) {
        logging::error{ "Could not create thumbnail filter: {}", err2str(ret) };
        return {};
    }

    AVFilterContextPtr scale_context{ nullptr };
    if (width > 0 && height > 0) {
        const AVFilter* const scale_filter{ avfilter_get_by_name("scale") };
        if (scale_filter == nullptr) {
            logging::error{ "Could not find scale filter" };
            return {};
        }

        const std::string scale_args{ std::format("{}:{}:force_original_aspect_ratio=decrease", width, height) };

        if (const int ret{ avfilter_graph_create_filter(std::out_ptr(scale_context), scale_filter, "scale", scale_args.c_str(), nullptr, filter_graph.get()) }; ret < 0) {
            logging::error{ "Could not create scale filter: {}", err2str(ret) };
            return {};
        }
    }

    // Connect filters
    std::vector<AVFilterContext*> filter_context_list;
    if (scale_context != nullptr) {
        // buffer -> thumbnail -> scale -> buffersink
        filter_context_list.push_back(buffer_source_context.get());
        filter_context_list.push_back(thumbnail_context.get());
        filter_context_list.push_back(scale_context.get());
        filter_context_list.push_back(buffer_sink_context.get());
    } else {
        // buffer -> thumbnail -> buffersink
        filter_context_list.push_back(buffer_source_context.get());
        filter_context_list.push_back(thumbnail_context.get());
        filter_context_list.push_back(buffer_sink_context.get());
    }

    for (int ret{ -1 }; const auto& [source_context, dest_context] : std::views::zip(filter_context_list, std::views::drop(filter_context_list, 1))) {
        if ((ret = avfilter_link(source_context, 0, dest_context, 0)) < 0) {
            logging::error{ "Could not link filters: {}", err2str(ret) };
            return {};
        }
    }

    if (const int ret{ avfilter_graph_config(filter_graph.get(), nullptr) }; ret < 0) {
        logging::error{ "Error configuring filter graph: {}", err2str(ret) };
        return {};
    }

    const AVFramePtr input_frame{ av_frame_alloc() };
    if (input_frame == nullptr) {
        logging::error{ "Could not allocate frame" };
        return {};
    }

    const AVPacketPtr input_packet{ av_packet_alloc() };
    if (input_packet == nullptr) {
        logging::error{ "Could not allocate packet" };
        return {};
    }

    std::string image;
    bool end_of_stream{ false };
    while (!end_of_stream && image.empty()) {
        if (const int ret{ av_read_frame(format_context.get(), input_packet.get()) }; ret < 0) {
            logging::error{ "Error reading frame: {}", err2str(ret) };
            return {};
        }

        if (input_packet->stream_index != video_stream_index) {
            continue;
        }

        if (const int ret{ avcodec_send_packet(codec_context.get(), input_packet.get()) }; ret < 0) {
            logging::error{ "Could not send frame packet: {}", err2str(ret) };
            return {};
        }

        if (const int ret{ avcodec_receive_frame(codec_context.get(), input_frame.get()) }; ret == AVERROR(EAGAIN)) {
            continue;
        } else if (ret == AVERROR_EOF) {
            end_of_stream = true;
        } else if (ret < 0) {
            logging::error{ "Could not receive frame packet: {}", err2str(ret) };
            return {};
        }

        if (!end_of_stream) {
            logging::debug{
                "Frame {} (type={}, format={}) pts {} [DTS {}]",
                codec_context->frame_num,
                av_get_picture_type_char(input_frame->pict_type),
                input_frame->format,
                input_frame->pts,
                input_frame->pkt_dts
            };

            if (input_frame->format != AV_PIX_FMT_YUV420P) {
                logging::info{ "Warning: the generated file may not be a grayscale image, but could e.g. be just the R component if the video format is RGB" };
            }

            if (const int ret{ av_buffersrc_add_frame_flags(buffer_source_context.get(), input_frame.get(), AV_BUFFERSRC_FLAG_KEEP_REF) }; ret < 0) {
                logging::error{ "Error feeding the filter chain: {}", err2str(ret) };
                return {};
            }
        } else {
            // force the last frame, close the buffer
            if (const int ret{ av_buffersrc_add_frame_flags(buffer_source_context.get(), nullptr, AV_BUFFERSRC_FLAG_PUSH) }; ret < 0) {
                logging::error{ "Error feeding the filter chain: {}", err2str(ret) };
                return {};
            }
        }

        AVFramePtr filtered_frame{ av_frame_alloc() };
        if (filtered_frame == nullptr) {
            logging::error{ "Could not allocate filtered frame" };
            return {};
        }

        if (const int ret{ av_buffersink_get_frame(buffer_sink_context.get(), filtered_frame.get()) }; ret == AVERROR(EAGAIN)) {
            continue;
        } else if (ret == AVERROR_EOF) {
            /* last frame */
        } else if (ret < 0) {
            logging::error{ "Error getting filtered frame: {}", err2str(ret) };
            return {};
        }

        logging::debug{
            "Thumbnail selected frame {} (type={}, format={})",
            codec_context->frame_num,
            av_get_picture_type_char(filtered_frame->pict_type),
            filtered_frame->format
        };

        // Calculate aspect ratio of the source frame
        if (filtered_frame->width != static_cast<int>(width) || filtered_frame->height != static_cast<int>(height)) {
            AVFramePtr padded_frame{ av_frame_alloc() };
            if (padded_frame == nullptr) {
                logging::error{ "Could not allocate padded frame" };
                return {};
            }

            // Set the properties of the padded AVFrame
            padded_frame->format = filtered_frame->format;
            padded_frame->width = width;
            padded_frame->height = height;

            if (const int ret{ av_image_alloc(padded_frame->data, padded_frame->linesize, static_cast<int>(width), static_cast<int>(height), static_cast<AVPixelFormat>(filtered_frame->format), 32) }; ret < 0) {
                logging::error{ "Could not allocate padded frame: {}", err2str(ret) };
                return {};
            }

            // fill with black
            for (std::size_t y{ 0 }; y < height; y++) {
                std::memset(padded_frame->data[0] + y * padded_frame->linesize[0], 0, width * 3);
            }

            // copy image to center
            const std::size_t x_offset{ (width - filtered_frame->width) / 2 };
            const std::size_t y_offset{ (height - filtered_frame->height) / 2 };
            for (int y{ 0 }; y < filtered_frame->height; y++) {
                std::memcpy(padded_frame->data[0] + ((y + y_offset) * padded_frame->linesize[0]) + (x_offset * 3),
                            filtered_frame->data[0] + (y * filtered_frame->linesize[0]),
                            filtered_frame->width * 3);
            }

            std::swap(filtered_frame, padded_frame);
        }

        image = frame_to_png(filtered_frame.get());
    }

    return image;
}

inline void video::AVDeleter::operator()(AVFormatContext* format_context) const
{
    avformat_close_input(&format_context);
}

inline void video::AVDeleter::operator()(AVCodecContext* codex_context) const
{
    avcodec_free_context(&codex_context);
}

inline void video::AVDeleter::operator()(AVFrame* frame) const
{
    av_frame_free(&frame);
}

inline void video::AVDeleter::operator()(AVIOContext* avio_context) const
{
    if (avio_context != nullptr)
        av_freep(&avio_context->buffer); // NOLINT(bugprone-multi-level-implicit-pointer-conversion): incorrect FFmpeg API
    avio_context_free(&avio_context);
}

inline void video::AVDeleter::operator()(AVFilterGraph* filter_graph) const
{
    avfilter_graph_free(&filter_graph);
}

inline void video::AVDeleter::operator()(AVFilterContext* filter_context) const
{
    avfilter_free(filter_context); // call av_free(filter_context)
    // delete filter_context;
}

inline void video::AVDeleter::operator()(AVPacket* packet) const
{
    if (packet != nullptr)
        av_packet_unref(packet);
    av_packet_free(&packet);
}

inline const char* video::err2str(int error_numero)
{
    static std::array<char, AV_ERROR_MAX_STRING_SIZE> error_buffer{};
    return av_make_error_string(error_buffer.data(), AV_ERROR_MAX_STRING_SIZE, error_numero);
}

inline int video::read_packet(void* opaque, std::uint8_t* buffer, int buffer_size)
{
    BufferData* const buffer_data{ static_cast<BufferData*>(opaque) };
    buffer_size = FFMIN(static_cast<std::size_t>(buffer_size), buffer_data->pointer.size());

    if (buffer_size <= 0)
        return AVERROR_EOF;

    // logging::debug{ "ptr: {} size: {}", reinterpret_cast<std::uintptr_t>(buffer_data->pointer.data()), buffer_data->pointer.size() };

    // copy internal buffer data to avio buffer
    std::memcpy(buffer, buffer_data->pointer.data(), buffer_size);
    buffer_data->pointer = buffer_data->pointer.subspan(buffer_size);

    return buffer_size;
}

inline std::int64_t video::seek(void* opaque, std::int64_t offset, int whence)
{
    BufferData* const buffer_data{ static_cast<BufferData*>(opaque) };
    std::int64_t pos{ -1 };

    switch (whence) {
        default:
        case AVSEEK_SIZE:
            pos = static_cast<std::int64_t>(buffer_data->ref_pointer.size());
            break;
        case SEEK_SET:
            buffer_data->pointer = buffer_data->ref_pointer.subspan(offset);
            pos = offset;
            break;
    }

    return pos;
}

inline std::string video::frame_to_png(const AVFrame* frame)
{
    png_structp png_ptr{ png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr) };
    if (png_ptr == nullptr) {
        logging::error{ "Could not create PNG writer" };
        return {};
    }

    png_infop info_ptr{ png_create_info_struct(png_ptr) };
    if (info_ptr == nullptr) {
        png_destroy_write_struct(&png_ptr, nullptr);
        logging::error{ "Could not create PNG info" };
        return {};
    }

    if (setjmp(png_jmpbuf(png_ptr))) { // NOLINT(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        png_destroy_write_struct(&png_ptr, &info_ptr);
        logging::error{ "Could not create PNG image" };
        return {};
    }

    std::ostringstream png_stream;
    png_set_write_fn(png_ptr, &png_stream, write_data, nullptr);

    constexpr int bit_depth{ 8 };
    png_set_IHDR(png_ptr, info_ptr, frame->width, frame->height, bit_depth, PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    std::vector<png_bytep> raw_pointers(frame->height);
    std::ranges::generate(raw_pointers, [pos{ 0 }, frame]() mutable -> png_bytep {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        return frame->data[0] + (static_cast<std::ptrdiff_t>(pos++) * frame->linesize[0]);
    });

    png_set_rows(png_ptr, info_ptr, raw_pointers.data());
    png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, nullptr);

    png_destroy_write_struct(&png_ptr, &info_ptr);

    return png_stream.str();
}

inline void video::write_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
    std::ostringstream* out_stream{ static_cast<std::ostringstream*>(png_get_io_ptr(png_ptr)) };
    out_stream->write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(length)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
}
