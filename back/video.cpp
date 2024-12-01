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
        void operator()(AVFormatContext* ctx) const noexcept { avformat_close_input(&ctx); }
        void operator()(AVCodecContext* ctx) const noexcept { avcodec_free_context(&ctx); }
        void operator()(AVFrame* frame) const noexcept { av_frame_free(&frame); }
        void operator()(AVPacket* pkt) const noexcept
        {
            av_packet_unref(pkt);
            av_packet_free(&pkt);
        }
        void operator()(SwsContext* ctx) const noexcept { sws_freeContext(ctx); }
        void operator()(AVIOContext* ctx) const noexcept
        {
            av_freep(&ctx->buffer);
            avio_context_free(&ctx);
        }
        void operator()(std::uint8_t* buffer) const noexcept { av_free(buffer); }
    };

    using AVFormatContextPtr = std::unique_ptr<AVFormatContext, AVDeleter>;
    using AVCodecContextPtr = std::unique_ptr<AVCodecContext, AVDeleter>;
    using AVFramePtr = std::unique_ptr<AVFrame, AVDeleter>;
    using AVPacketPtr = std::unique_ptr<AVPacket, AVDeleter>;
    using SwsContextPtr = std::unique_ptr<SwsContext, AVDeleter>;
    using AVIOContextPtr = std::unique_ptr<AVIOContext, AVDeleter>;
    using BufferPtr = std::unique_ptr<std::uint8_t, AVDeleter>;

    const char* err2str(int error_numero) noexcept;

    using BufferData = std::span<char const>;
    int read_packet(void* opaque, std::uint8_t* buf, int buf_size) noexcept;

    std::string frame_to_png(const AVFrame* frame, int width, int height) noexcept;
    void write_data(png_structp png_ptr, png_bytep data, png_size_t length) noexcept;
}

std::string video::extract_first_frame(const std::string& video_content, const std::string& format, std::int64_t timestamp) noexcept
{
    AVFormatContext* raw_fmt_ctx(avformat_alloc_context());
    if (!raw_fmt_ctx) {
        logging::error{ "Could not allocate format context" };
        return {};
    }

    constexpr std::size_t ctx_size{ 4096 };
    std::uint8_t* avio_ctx_buffer{ reinterpret_cast<std::uint8_t*>(av_malloc(ctx_size)) };
    if (!avio_ctx_buffer) {
        logging::error{ "Could not allocate context buffer" };
        avformat_free_context(raw_fmt_ctx);
        return {};
    }

    // fill opaque structure used by the AVIOContext read callback
    BufferData bd{ video_content };

    AVIOContextPtr avio_ctx(avio_alloc_context(avio_ctx_buffer, ctx_size, 0, &bd, read_packet, nullptr, nullptr));
    if (!avio_ctx) {
        logging::error{ "Could not read context buffer" };
        av_free(avio_ctx_buffer);
        avformat_free_context(raw_fmt_ctx);
        return {};
    }

    raw_fmt_ctx->pb = avio_ctx.get();

    // Explicitly set the input format to MP4
    const AVInputFormat* input_format{ av_find_input_format(format.c_str()) };
    if (!input_format) {
        logging::error{ "Could not found format \"{}\"", format };
        return {};
    }

    // Set options for probing
    AVDictionary* opts{ nullptr };
    av_dict_set(&opts, "analyzeduration", std::to_string(video_content.size()).c_str(), 0); // Increase analyzeduration to 5 seconds
    av_dict_set(&opts, "probesize", std::to_string(video_content.size()).c_str(), 0);       // Increase probesize to 5 MB

    // Open the input video
    if (const int ret{ avformat_open_input(&raw_fmt_ctx, nullptr, input_format, &opts) }; ret < 0) {
        logging::error{ "Could not open input: {}", err2str(ret) };
        av_dict_free(&opts);
        return {};
    }

    av_dict_free(&opts);

    AVFormatContextPtr fmt_ctx(raw_fmt_ctx);

    if (const int ret{ avformat_find_stream_info(fmt_ctx.get(), nullptr) }; ret < 0) {
        logging::error{ "Could not find stream information: {}", err2str(ret) };
        return {};
    }

#ifdef DEBUG_LOG
    // Dump information about file onto standard error
    av_dump_format(raw_fmt_ctx, 0, nullptr, false);
#endif

    int video_stream_idx{ -1 };
    for (unsigned i = 0; i < fmt_ctx->nb_streams && video_stream_idx == -1; ++i) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_idx = i;
        }
    }

    if (video_stream_idx == -1) {
        logging::error{ "Could not find video stream" };
        return {};
    }

    const AVCodecParameters* codecpar{ fmt_ctx->streams[video_stream_idx]->codecpar };

    const AVCodec* codec{ avcodec_find_decoder(codecpar->codec_id) };
    if (codec == nullptr) {
        logging::error{ "Could not find codec" };
        return {};
    }

    AVCodecContextPtr codec_ctx(avcodec_alloc_context3(codec));
    if (!codec_ctx) {
        logging::error{ "Could not allocate codec context" };
        return {};
    }

    if (const int ret{ avcodec_parameters_to_context(codec_ctx.get(), codecpar) }; ret < 0) {
        logging::error{ "Could not set codec parameters: {}", err2str(ret) };
        return {};
    }
    if (const int ret{ avcodec_open2(codec_ctx.get(), codec, nullptr) }; ret < 0) {
        logging::error{ "Could not open codec: {}", err2str(ret) };
        return {};
    }

    if (const int ret{ av_seek_frame(fmt_ctx.get(), video_stream_idx, timestamp * AV_TIME_BASE, AVSEEK_FLAG_BACKWARD) }; ret < 0) {
        logging::error{ "Could not seek to {}s: {}", timestamp, err2str(ret) };
        return {};
    }

    AVFramePtr frame(av_frame_alloc());
    AVFramePtr frame_rgb(av_frame_alloc());
    if (!frame || !frame_rgb) {
        logging::error{ "Could not allocate frames" };
        return {};
    }

    const int num_bytes{ av_image_get_buffer_size(AV_PIX_FMT_RGB24, codec_ctx->width, codec_ctx->height, 1) };
    BufferPtr buffer{ reinterpret_cast<std::uint8_t*>(av_malloc(num_bytes)) };
    if (!buffer) {
        logging::error{ "Could not allocate image buffer" };
        return {};
    }

    if (const int ret{ av_image_fill_arrays(frame_rgb->data, frame_rgb->linesize, buffer.get(), AV_PIX_FMT_RGB24, codec_ctx->width, codec_ctx->height, 1) }; ret < 0) {
        logging::error{ "Could not fill image: {}", err2str(ret) };
        return {};
    }

    SwsContextPtr sws_ctx(sws_getContext(codec_ctx->width, codec_ctx->height, codec_ctx->pix_fmt, codec_ctx->width, codec_ctx->height, AV_PIX_FMT_RGB24, SWS_BILINEAR, nullptr, nullptr, nullptr));
    if (!sws_ctx) {
        logging::error{ "Could not allocate color conversion context" };
        return {};
    }

    AVPacketPtr pkt(av_packet_alloc());
    if (!pkt) {
        logging::error{ "Could not allocate packet" };
        return {};
    }

    std::string image;
    while (av_read_frame(fmt_ctx.get(), pkt.get()) >= 0 && image.empty()) {
        if (pkt->stream_index != video_stream_idx) {
            continue;
        }

        if (const int ret{ avcodec_send_packet(codec_ctx.get(), pkt.get()) }; ret < 0) {
            logging::error{ "Could not send frame packet: {}", err2str(ret) };
            return {};
        }

        if (int ret{ avcodec_receive_frame(codec_ctx.get(), frame.get()) }; ret < 0) {
            if (ret == AVERROR(EAGAIN)) {
                // Need more packets to produce a frame
                continue;
            }

            logging::error{ "Could not receive frame packet: {}", err2str(ret) };
            return {};
        }

        if (const int ret{ sws_scale(sws_ctx.get(), frame->data, frame->linesize, 0, codec_ctx->height, frame_rgb->data, frame_rgb->linesize) }; ret < 0) {
            logging::error{ "Could not encode frame to image: {}", err2str(ret) };
            return {};
        }

        image = frame_to_png(frame_rgb.get(), codec_ctx->width, codec_ctx->height);
    }

    return image;
}

inline const char* video::err2str(int error_numero) noexcept
{
    static std::array<char, AV_ERROR_MAX_STRING_SIZE> error_buffer{};
    return av_make_error_string(error_buffer.data(), AV_ERROR_MAX_STRING_SIZE, error_numero);
}

inline int video::read_packet(void* opaque, std::uint8_t* buf, int buf_size) noexcept
{
    BufferData* bd{ static_cast<BufferData*>(opaque) };
    buf_size = FFMIN(buf_size, static_cast<int>(bd->size()));
    if (buf_size <= 0)
        return AVERROR_EOF;

    // logging::debug{ "ptr: {}, size: {}, copy: {}", bd->data()[0], bd->size(), buf_size };

    // copy internal buffer data to buf
    std::memcpy(buf, bd->data(), buf_size);
    *bd = bd->subspan(buf_size);
    return buf_size;
}

inline std::string video::frame_to_png(const AVFrame* frame, int width, int height) noexcept
{
    png_structp png_ptr{ png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr) };
    if (!png_ptr) {
        logging::error{ "Could not create PNG writer" };
        return {};
    }

    png_infop info_ptr{ png_create_info_struct(png_ptr) };
    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr, nullptr);
        logging::error{ "Could not create PNG info" };
        return {};
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        logging::error{ "Could not create PNG image" };
        return {};
    }

    std::ostringstream png_stream;
    png_set_write_fn(png_ptr, &png_stream, write_data, nullptr);

    png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    std::vector<png_bytep> row_pointers(height);
    for (std::size_t y{ 0 }; y < static_cast<std::size_t>(height); ++y) {
        row_pointers[y] = frame->data[0] + y * frame->linesize[0];
    }

    png_set_rows(png_ptr, info_ptr, row_pointers.data());
    png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, nullptr);

    png_destroy_write_struct(&png_ptr, &info_ptr);

    return png_stream.str();
}

inline void video::write_data(png_structp png_ptr, png_bytep data, png_size_t length) noexcept
{
    std::ostringstream* out_stream{ reinterpret_cast<std::ostringstream*>(png_get_io_ptr(png_ptr)) };
    out_stream->write(reinterpret_cast<const char*>(data), length);
}
