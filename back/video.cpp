#include "video.h"

#include "logging.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/mem.h>
#include <libswscale/swscale.h>

#include <png.h>
}

#include <algorithm>
#include <cstring>
#include <memory>
#include <span>
#include <vector>

namespace video
{
    struct AVDeleter
    {
        void operator()(AVFormatContext* format_context) const;
        void operator()(AVCodecContext* codex_context) const;
        void operator()(AVFrame* frame) const;
        void operator()(AVIOContext* avio_context) const;
        void operator()(AVPacket* packet) const;
        void operator()(SwsContext* color_context) const;
    };

    using AVFormatContextPtr = std::unique_ptr<AVFormatContext, AVDeleter>;
    using AVCodecContextPtr = std::unique_ptr<AVCodecContext, AVDeleter>;
    using AVFramePtr = std::unique_ptr<AVFrame, AVDeleter>;
    using AVIOContextPtr = std::unique_ptr<AVIOContext, AVDeleter>;
    using AVPacketPtr = std::unique_ptr<AVPacket, AVDeleter>;
    using SwsContextPtr = std::unique_ptr<SwsContext, AVDeleter>;

    const char* err2str(int error_numero);

    struct BufferData
    {
        std::span<char const> pointer;
        const std::span<char const> ref_pointer; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
    };
    int read_packet(void* opaque, std::uint8_t* buffer, int buffer_size);
    std::int64_t seek(void* opaque, std::int64_t offset, int whence);

    std::string decode_packet(const AVPacket* packet, AVCodecContext* codec_context, AVFrame* frame, std::size_t width, std::size_t height);
    std::string frame_to_png(const AVFrame* frame);
    void write_data(png_structp png_ptr, png_bytep data, png_size_t length);
}

std::string video::extract_first_frame(const std::string& video_content, const std::string& format, std::int64_t timestamp,
                                       std::size_t width, std::size_t height)
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
    const AVCodecParameters* input_codec_parameters{ nullptr };
    int video_stream_index{ -1 };
    // Loop though all the streams and print its main information
    for (unsigned i{ 0 }; i < format_context->nb_streams; i++) {
        // NOLINTBEGIN(cppcoreguidelines-pro-bounds-array-to-pointer-decay, cppcoreguidelines-pro-bounds-pointer-arithmetic)
        const AVCodecParameters* const local_codec_parameters{ format_context->streams[i]->codecpar };
        logging::debug{ "    AVStream->time_base before open coded {}/{}", format_context->streams[i]->time_base.num, format_context->streams[i]->time_base.den };
        logging::debug{ "    AVStream->r_frame_rate before open coded {}/{}", format_context->streams[i]->r_frame_rate.num, format_context->streams[i]->r_frame_rate.den };
        logging::debug{ "    AVStream->start_time {}", format_context->streams[i]->start_time };
        logging::debug{ "    AVStream->duration {}", format_context->streams[i]->duration };

        const AVCodec* const local_codec{ avcodec_find_decoder(local_codec_parameters->codec_id) };
        if (local_codec == nullptr) {
            logging::error{ "Could not find codec: {}", static_cast<int>(local_codec_parameters->codec_id) };
            continue;
        }
        // NOLINTEND(cppcoreguidelines-pro-bounds-array-to-pointer-decay, cppcoreguidelines-pro-bounds-pointer-arithmetic)

        if (local_codec_parameters->codec_type == AVMEDIA_TYPE_VIDEO && video_stream_index == -1) {
            video_stream_index = static_cast<int>(i);
            input_codec = local_codec;
            input_codec_parameters = local_codec_parameters;

            logging::debug{ "Video Codec: resolution {} x {}", local_codec_parameters->width, local_codec_parameters->height };
        } else if (local_codec_parameters->codec_type == AVMEDIA_TYPE_AUDIO) {
            logging::debug{ "Audio Codec: {} channels, sample rate {} bits/s", local_codec_parameters->ch_layout.nb_channels, local_codec_parameters->sample_rate };
        }

        logging::debug{ "    Codec {} ID {} bit_rate {} bits/s", local_codec->name, static_cast<int>(local_codec->id), local_codec_parameters->bit_rate };
    }

    if (video_stream_index == -1) {
        logging::error{ "Could not find video stream" };
        return {};
    }

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

    if (const int ret{ av_seek_frame(format_context.get(), video_stream_index, timestamp * AV_TIME_BASE, AVSEEK_FLAG_BACKWARD) }; ret < 0) {
        logging::error{ "Could not seek to {}s: {}", timestamp, err2str(ret) };
        return {};
    }

    const AVFramePtr input_frame(av_frame_alloc());
    if (input_frame == nullptr) {
        logging::error{ "Could not allocate frame" };
        return {};
    }

    const AVPacketPtr input_packet(av_packet_alloc());
    if (input_packet == nullptr) {
        logging::error{ "Could not allocate packet" };
        return {};
    }

    std::string image;
    while (av_read_frame(format_context.get(), input_packet.get()) >= 0 && image.empty()) {
        if (input_packet->stream_index != video_stream_index) {
            continue;
        }

        image = decode_packet(input_packet.get(), codec_context.get(), input_frame.get(), width, height);
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

inline void video::AVDeleter::operator()(AVPacket* packet) const
{
    if (packet != nullptr)
        av_packet_unref(packet);
    av_packet_free(&packet);
}

inline void video::AVDeleter::operator()(SwsContext* color_context) const
{
    sws_freeContext(color_context);
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

inline std::string video::decode_packet(const AVPacket* packet, AVCodecContext* codec_context, AVFrame* frame, std::size_t width, std::size_t height)
{
    if (const int ret{ avcodec_send_packet(codec_context, packet) }; ret < 0) {
        logging::error{ "Could not send frame packet: {}", err2str(ret) };
        return {};
    }

    const int ret{ avcodec_receive_frame(codec_context, frame) };
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        return {};
    }

    if (ret < 0) {
        logging::error{ "Could not receive frame packet: {}", err2str(ret) };
        return {};
    }

    logging::debug{
        "Frame {} (type={}, format={}) pts {} [DTS {}]",
        codec_context->frame_num,
        av_get_picture_type_char(frame->pict_type),
        frame->format,
        frame->pts,
        frame->pkt_dts
    };

    if (frame->format != AV_PIX_FMT_YUV420P) {
        logging::info{ "Warning: the generated file may not be a grayscale image, but could e.g. be just the R component if the video format is RGB" };
    }

    // Calculate aspect ratio of the source frame
    const double src_aspect_ratio{ static_cast<double>(frame->width) / static_cast<double>(frame->height) };
    const double target_aspect_ratio{ static_cast<double>(width) / static_cast<double>(height) };

    std::size_t scaled_width{ 0 };
    std::size_t scaled_height{ 0 };
    std::size_t x_offset{ 0 };
    std::size_t y_offset{ 0 };

    if (src_aspect_ratio > target_aspect_ratio) {
        // Fit width, add padding to height
        scaled_width = width;
        scaled_height = static_cast<std::size_t>(static_cast<double>(width) / src_aspect_ratio);
        y_offset = (height - scaled_height) / 2;
    } else {
        // Fit height, add padding to width
        scaled_width = static_cast<std::size_t>(static_cast<double>(height) * src_aspect_ratio);
        scaled_height = height;
        x_offset = (width - scaled_width) / 2;
    }

    const SwsContextPtr color_context(
        sws_getContext(codec_context->width, codec_context->height, codec_context->pix_fmt,
                       static_cast<int>(scaled_width), static_cast<int>(scaled_height), AV_PIX_FMT_RGB24, SWS_BICUBIC, nullptr, nullptr, nullptr));
    if (color_context == nullptr) {
        logging::error{ "Could not allocate color conversion context" };
        return {};
    }

    const AVFramePtr rgb_frame(av_frame_alloc());
    if (rgb_frame == nullptr) {
        logging::error{ "Could not allocate RGB frame" };
        return {};
    }

    // Set the properties of the output AVFrame
    rgb_frame->format = AV_PIX_FMT_RGB24;
    rgb_frame->width = static_cast<int>(scaled_width);
    rgb_frame->height = static_cast<int>(scaled_height);

    if (const int ret{ av_frame_get_buffer(rgb_frame.get(), 0) }; ret < 0) {
        logging::error{ "Could not prepare RGB frame: {}", err2str(ret) };
        return {};
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
    if (const int ret{ sws_scale(color_context.get(), frame->data, frame->linesize, 0, frame->height, rgb_frame->data, rgb_frame->linesize) }; ret < 0) {
        logging::error{ "Could not translate the frame format from YUV420P into RGB24: {}", err2str(ret) };
        return {};
    }

    const AVFramePtr final_rgb_frame(av_frame_alloc());
    if (final_rgb_frame == nullptr) {
        logging::error{ "Could not allocate final RGB frame" };
        return {};
    }

    // Copy scaled content into padded frame
    final_rgb_frame->format = AV_PIX_FMT_RGB24;
    final_rgb_frame->width = static_cast<int>(width);
    final_rgb_frame->height = static_cast<int>(height);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
    if (const int ret{ av_image_alloc(final_rgb_frame->data, final_rgb_frame->linesize, static_cast<int>(width), static_cast<int>(height), AV_PIX_FMT_RGB24, 32) }; ret < 0) {
        logging::error{ "Could not allocate final image data RGB frame: {}", err2str(ret) };
        return {};
    }

    for (std::size_t y{ 0 }; y < scaled_height; ++y) {
        // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        std::memcpy(final_rgb_frame->data[0] + ((y + y_offset) * final_rgb_frame->linesize[0]) + (x_offset * 3), // For RGB24
                    rgb_frame->data[0] + (y * rgb_frame->linesize[0]),
                    scaled_width * 3);
        // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }

    return frame_to_png(final_rgb_frame.get());
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
