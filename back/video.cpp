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
#include <fstream>
#include <memory>
#include <ranges>
#include <span>
#include <utility>
#include <vector>

namespace video
{
    struct AVOutputFormatDeleter
    {
        void operator()(AVFormatContext* format_context) const;
    };

    using AVOutputFormatContextPtr = std::unique_ptr<AVFormatContext, AVOutputFormatDeleter>;

    struct AVDeleter
    {
        void operator()(AVFormatContext* format_context) const;
        void operator()(AVCodecContext* codex_context) const;
        void operator()(AVFrame* frame) const;
        void operator()(AVIOContext* avio_context) const;
        void operator()(AVFilterGraph* filter_graph) const;
        void operator()(AVFilterContext* filter_context) const;
        void operator()(AVPacket* packet) const;
        void operator()(AVDictionary* opts) const;
    };

    using AVFormatContextPtr = std::unique_ptr<AVFormatContext, AVDeleter>;
    using AVCodecContextPtr = std::unique_ptr<AVCodecContext, AVDeleter>;
    using AVFramePtr = std::unique_ptr<AVFrame, AVDeleter>;
    using AVIOContextPtr = std::unique_ptr<AVIOContext, AVDeleter>;
    using AVFilterGraphPtr = std::unique_ptr<AVFilterGraph, AVDeleter>;
    using AVFilterContextPtr = std::unique_ptr<AVFilterContext, AVDeleter>;
    using AVPacketPtr = std::unique_ptr<AVPacket, AVDeleter>;
    using AVDictionaryPtr = std::unique_ptr<AVDictionary, AVDeleter>;

    constexpr const char* frame_id_key() { return "id"; }
    const char* err2str(int error_numero);

    struct BufferData
    {
        std::span<char const> pointer;
        std::span<char const> ref_pointer;
    };
    int read_packet(void* opaque, std::uint8_t* buffer, int buffer_size);
    std::int64_t seek(void* opaque, std::int64_t offset, int whence);

    std::string frame_to_png(const AVFrame* frame);
    void write_data(png_structp png_ptr, png_bytep data, png_size_t length);

    // Custom IO context for HLS output — owns the ofstream for each file opened by the muxer
    struct HlsIoContext
    {
        std::ofstream stream;
    };

    int hls_io_write_packet(void* opaque, const std::uint8_t* buf, int buf_size);
    int hls_io_open(AVFormatContext* ctx, AVIOContext** pb, const char* url, int flags, AVDictionary** options);
    int hls_io_close(AVFormatContext* ctx, AVIOContext* pb);

    class VideoProcessor
    {
    public:
        std::string generate_thumbnail(const std::string& video_content, const std::string& format,
                                       std::size_t width, std::size_t height, std::size_t n_images);

        bool convert_to_hls(const std::string& video_content, const std::string& out_dir, const std::string& name,
                            const std::string& format, int hls_time);

    protected:
        bool initialize(const std::string& video_content, const std::string& format);
        bool open_input_stream();
        bool setup_video_decoder();
        bool create_filter_graph(std::size_t width, std::size_t height, std::size_t n_images);
        std::string process_frames(std::size_t width, std::size_t height);
        std::string retrieve_filtered_frame(std::size_t width, std::size_t height);
        bool remux_to_hls(const std::string& out_dir, const std::string& name, int hls_time);

    private:
        BufferData _buffer_data;
        AVFormatContextPtr _format_context{ nullptr };
        AVIOContextPtr _avio_context{ nullptr };
        AVCodecContextPtr _codec_context{ nullptr };
        AVFilterGraphPtr _filter_graph{ nullptr };
        AVFilterContextPtr _buffer_source_context{ nullptr };
        AVFilterContextPtr _thumbnail_context{ nullptr };
        AVFilterContextPtr _scale_context{ nullptr };
        AVFilterContextPtr _buffer_sink_context{ nullptr };
        int _video_stream_index{ -1 };
        const AVStream* _video_stream{ nullptr };
    };
}

std::string video::thumbnail(const std::string& video_content, const std::string& format,
                             std::size_t width, std::size_t height, std::size_t n_images)
{
    VideoProcessor processor;
    return processor.generate_thumbnail(video_content, format, width, height, n_images);
}

bool video::convert_to_hls(const std::string& video_content, const std::string& out_dir, const std::string& name,
                           const std::string& format, int hls_time)
{
    VideoProcessor processor;
    return processor.convert_to_hls(video_content, out_dir, name, format, hls_time);
}

bool video::VideoProcessor::convert_to_hls(const std::string& video_content, const std::string& out_dir, const std::string& name,
                                           const std::string& format, int hls_time)
{
#ifndef NDEBUG
    av_log_set_level(AV_LOG_DEBUG);
#endif

    if (!initialize(video_content, format)) {
        return false;
    }

    if (!open_input_stream()) {
        return false;
    }

    return remux_to_hls(out_dir, name, hls_time);
}

inline std::string video::VideoProcessor::generate_thumbnail(const std::string& video_content, const std::string& format,
                                                             std::size_t width, std::size_t height, std::size_t n_images)
{
#ifndef NDEBUG
    av_log_set_level(AV_LOG_DEBUG);
#endif

    if (!initialize(video_content, format)) {
        return {};
    }

    if (!open_input_stream()) {
        return {};
    }

    if (!setup_video_decoder()) {
        return {};
    }

    if (!create_filter_graph(width, height, n_images)) {
        return {};
    }

    return process_frames(width, height);
}

inline bool video::VideoProcessor::initialize(const std::string& video_content, const std::string& format)
{
    _format_context.reset(avformat_alloc_context());
    if (_format_context == nullptr) {
        logging::error{ "Could not allocate format context" };
        return false;
    }

    // fill opaque structure used by the AVIOContext read callback
    _buffer_data.pointer = video_content;
    _buffer_data.ref_pointer = video_content;

    constexpr std::size_t avio_context_buffer_size{ 4096 };
    std::uint8_t* const avio_context_buffer{ static_cast<std::uint8_t*>(av_malloc(avio_context_buffer_size)) };
    if (avio_context_buffer == nullptr) {
        logging::error{ "Could not allocate avio context buffer" };
        return false;
    }

    _avio_context.reset(avio_alloc_context(avio_context_buffer, avio_context_buffer_size, 0, &_buffer_data, &read_packet, nullptr, &seek));
    if (_avio_context == nullptr) {
        logging::error{ "Could not allocate avio context" };
        return false;
    }

    _format_context->pb = _avio_context.get();
    _format_context->flags = AVFMT_FLAG_CUSTOM_IO;
    _format_context->iformat = av_find_input_format(format.c_str()); // not necessary
    constexpr std::int64_t probe_size{ 1200000 };
    _format_context->probesize = probe_size;

    return true;
}

inline bool video::VideoProcessor::open_input_stream()
{
    // Open the input video
    if (const int ret{ avformat_open_input(std::inout_ptr(_format_context), nullptr, nullptr, nullptr) }; ret < 0) {
        logging::error{ "Could not open input: {}", err2str(ret) };
        return false;
    }

    logging::debug{ "Format: {}, Duration: {} us, Bitrate: {} bits/s", _format_context->iformat->name, _format_context->duration, _format_context->bit_rate };

    if (const int ret{ avformat_find_stream_info(_format_context.get(), nullptr) }; ret < 0) {
        logging::error{ "Could not find stream information: {}", err2str(ret) };
        return false;
    }

#ifdef DEBUG_LOG
    // Dump information about file onto standard error
    av_dump_format(_format_context.get(), 0, nullptr, 0);
#endif

    if (_format_context->pb != nullptr && _format_context->pb->error != 0) {
        logging::error{ "Error reading video content: {}", err2str(_format_context->pb->error) };
        return false;
    }

    return true;
}

inline bool video::VideoProcessor::setup_video_decoder()
{
    const AVCodec* input_codec{ nullptr };
    _video_stream_index = av_find_best_stream(_format_context.get(), AVMEDIA_TYPE_VIDEO, -1, -1, &input_codec, 0);
    if (_video_stream_index < 0) {
        logging::error{ "Could not find video stream" };
        return false;
    }

    const std::span streams(_format_context->streams, _format_context->nb_streams);
    _video_stream = streams[_video_stream_index];

    const AVCodecParameters* const input_codec_parameters{ _video_stream->codecpar };

    _codec_context.reset(avcodec_alloc_context3(input_codec));
    if (_codec_context == nullptr) {
        logging::error{ "Could not allocate codec context" };
        return false;
    }

    if (const int ret{ avcodec_parameters_to_context(_codec_context.get(), input_codec_parameters) }; ret < 0) {
        logging::error{ "Could not set codec parameters: {}", err2str(ret) };
        return false;
    }

    if (const int ret{ avcodec_open2(_codec_context.get(), input_codec, nullptr) }; ret < 0) {
        logging::error{ "Could not open codec: {}", err2str(ret) };
        return false;
    }

    return true;
}

inline bool video::VideoProcessor::create_filter_graph(std::size_t width, std::size_t height, std::size_t n_images)
{
    _filter_graph.reset(avfilter_graph_alloc());
    if (_filter_graph == nullptr) {
        logging::error{ "Could not allocate filter graph" };
        return false;
    }

    const AVFilter* const buffer_source{ avfilter_get_by_name("buffer") };
    if (buffer_source == nullptr) {
        logging::error{ "Could not find buffer source filter" };
        return false;
    }

    const std::string filter_args{ std::format(
        "video_size={}x{}:pix_fmt={}:time_base={}/{}:pixel_aspect={}/{}",
        _codec_context->width, _codec_context->height, std::to_underlying(_codec_context->pix_fmt),
        _video_stream->time_base.num, _video_stream->time_base.den,
        _codec_context->sample_aspect_ratio.num, _codec_context->sample_aspect_ratio.den) };

    if (const int ret{ avfilter_graph_create_filter(std::out_ptr(_buffer_source_context), buffer_source, "in", filter_args.c_str(), nullptr, _filter_graph.get()) }; ret < 0) {
        logging::error{ "Cannot create buffer source: {}", err2str(ret) };
        return false;
    }

    const AVFilter* const buffer_sink{ avfilter_get_by_name("buffersink") };
    if (buffer_sink == nullptr) {
        logging::error{ "Could not find buffer sink filter" };
        return false;
    }

    _buffer_sink_context.reset(avfilter_graph_alloc_filter(_filter_graph.get(), buffer_sink, "out"));
    if (_buffer_sink_context == nullptr) {
        logging::error{ "Cannot create buffer sink" };
        return false;
    }

    constexpr std::array pixel_formats{ AV_PIX_FMT_RGB24 };
    if (const int ret{ av_opt_set_array(_buffer_sink_context.get(), "pixel_formats", AV_OPT_SEARCH_CHILDREN, 0, pixel_formats.size(), AV_OPT_TYPE_PIXEL_FMT, pixel_formats.data()) }; ret < 0) {
        logging::error{ "Cannot set output pixel format: {}", err2str(ret) };
        return false;
    }

    if (const int ret{ avfilter_init_dict(_buffer_sink_context.get(), nullptr) }; ret < 0) {
        logging::error{ "Cannot initialize buffer sink: {}", err2str(ret) };
        return false;
    }

    const AVFilter* thumbnail_filter{ avfilter_get_by_name("thumbnail") };
    if (thumbnail_filter == nullptr) {
        logging::error{ "Could not find thumbnail filter" };
        return false;
    }

    const std::string thumbnail_args{ std::format("n={}", n_images) };

    if (const int ret{ avfilter_graph_create_filter(std::out_ptr(_thumbnail_context), thumbnail_filter, "thumbnail", thumbnail_args.c_str(), nullptr, _filter_graph.get()) }; ret < 0) {
        logging::error{ "Could not create thumbnail filter: {}", err2str(ret) };
        return false;
    }

    if (width > 0 && height > 0) {
        const AVFilter* const scale_filter{ avfilter_get_by_name("scale") };
        if (scale_filter == nullptr) {
            logging::error{ "Could not find scale filter" };
            return false;
        }

        const std::string scale_args{ std::format("{}:{}:force_original_aspect_ratio=decrease", width, height) };

        if (const int ret{ avfilter_graph_create_filter(std::out_ptr(_scale_context), scale_filter, "scale", scale_args.c_str(), nullptr, _filter_graph.get()) }; ret < 0) {
            logging::error{ "Could not create scale filter: {}", err2str(ret) };
            return false;
        }
    }

    std::vector<AVFilterContext*> filter_context_list;
    if (_scale_context != nullptr) {
        // buffer -> thumbnail -> scale -> buffersink
        filter_context_list = {
            _buffer_source_context.get(),
            _thumbnail_context.get(),
            _scale_context.get(),
            _buffer_sink_context.get()
        };
    } else {
        // buffer -> thumbnail -> buffersink
        filter_context_list = {
            _buffer_source_context.get(),
            _thumbnail_context.get(),
            _buffer_sink_context.get()
        };
    }

    for (int ret{ -1 }; const auto& [source_context, dest_context] : std::views::zip(filter_context_list, std::views::drop(filter_context_list, 1))) {
        ret = avfilter_link(source_context, 0, dest_context, 0);
        if (ret < 0) {
            logging::error{ "Could not link filters: {}", err2str(ret) };
            return false;
        }
    }

    if (const int ret{ avfilter_graph_config(_filter_graph.get(), nullptr) }; ret < 0) {
        logging::error{ "Error configuring filter graph: {}", err2str(ret) };
        return false;
    }

    return true;
}

inline std::string video::VideoProcessor::process_frames(std::size_t width, std::size_t height)
{
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
        if (const int ret{ av_read_frame(_format_context.get(), input_packet.get()) }; ret < 0) {
            logging::error{ "Error reading frame: {}", err2str(ret) };
            return {};
        }

        if (input_packet->stream_index != _video_stream_index) {
            continue;
        }

        if (const int ret{ avcodec_send_packet(_codec_context.get(), input_packet.get()) }; ret < 0) {
            logging::error{ "Could not send frame packet: {}", err2str(ret) };
            return {};
        }

        switch (const int ret{ avcodec_receive_frame(_codec_context.get(), input_frame.get()) }; ret) {
            case AVERROR(EAGAIN):
                continue;
            case AVERROR_EOF:
                end_of_stream = true;
                [[fallthrough]];
            case 0:
                break;
            default: // ret < 0
                logging::error{ "Could not receive frame packet: {}", err2str(ret) };
                return {};
        }

        if (!end_of_stream) {
            const std::int64_t frame_id{ _codec_context->frame_num - 1 };
            if (const int ret{ av_dict_set_int(&input_frame->metadata, frame_id_key(), frame_id, 0) }; ret < 0) {
                logging::error{ "Could not set frame id {} to dictionary entry {}: {}", frame_id, frame_id_key(), err2str(ret) };
                // continue (do not block for this)
            }

            logging::debug{
                "Frame {} (type={}, format={}) pts {} [DTS {}]",
                frame_id,
                av_get_picture_type_char(input_frame->pict_type),
                input_frame->format,
                input_frame->pts,
                input_frame->pkt_dts
            };

            if (input_frame->format != AV_PIX_FMT_YUV420P) {
                logging::info{ "Warning: the generated file may not be a grayscale image, but could e.g. be just the R component if the video format is RGB" };
            }

            if (const int ret{ av_buffersrc_add_frame_flags(_buffer_source_context.get(), input_frame.get(), AV_BUFFERSRC_FLAG_KEEP_REF) }; ret < 0) {
                logging::error{ "Error feeding the filter chain: {}", err2str(ret) };
                return {};
            }

            // force the last frame, close the buffer
        } else if (const int ret{ av_buffersrc_add_frame_flags(_buffer_source_context.get(), nullptr, AV_BUFFERSRC_FLAG_PUSH) }; ret < 0) {
            logging::error{ "Error feeding the filter chain: {}", err2str(ret) };
            return {};
        }

        image = retrieve_filtered_frame(width, height);
    }

    return image;
}

inline std::string video::VideoProcessor::retrieve_filtered_frame(std::size_t width, std::size_t height)
{
    const AVFramePtr filtered_frame{ av_frame_alloc() };
    if (filtered_frame == nullptr) {
        logging::error{ "Could not allocate filtered frame" };
        return {};
    }

    switch (const int ret{ av_buffersink_get_frame(_buffer_sink_context.get(), filtered_frame.get()) }; ret) {
        case AVERROR(EAGAIN):
            return {};
        case AVERROR_EOF: // last frame
        case 0:
            break;
        default: // ret < 0
            logging::error{ "Error getting filtered frame: {}", err2str(ret) };
            return {};
    }

    const AVDictionaryEntry* const tag{ av_dict_get(filtered_frame->metadata, frame_id_key(), nullptr, 0) };
    if (tag != nullptr) { // for diagnostic, is n_images enough?
        const char* const frame_id{ tag->value };
        logging::info{
            "Thumbnail selected frame {} (type={}, format={})",
            frame_id,
            av_get_picture_type_char(filtered_frame->pict_type),
            filtered_frame->format
        };
    }

    // Calculate aspect ratio of the source frame
    if (std::cmp_not_equal(filtered_frame->width, width) || std::cmp_not_equal(filtered_frame->height, height)) {
        const AVFramePtr padded_frame{ av_frame_alloc() };
        if (padded_frame == nullptr) {
            logging::error{ "Could not allocate padded frame" };
            return {};
        }

        // Set the properties of the padded AVFrame
        padded_frame->format = filtered_frame->format;
        padded_frame->width = static_cast<int>(width);
        padded_frame->height = static_cast<int>(height);

        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        if (const int ret{ av_image_alloc(padded_frame->data, padded_frame->linesize, static_cast<int>(width), static_cast<int>(height), static_cast<AVPixelFormat>(filtered_frame->format), 32) }; ret < 0) {
            logging::error{ "Could not allocate padded frame: {}", err2str(ret) };
            return {};
        }

        // Fill with black
        for (std::size_t y{ 0 }; y < height; y++) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            std::memset(padded_frame->data[0] + (y * padded_frame->linesize[0]), 0, width * 3);
        }

        // Copy image to center
        const std::size_t x_offset{ (width - static_cast<std::size_t>(filtered_frame->width)) / 2 };
        const std::size_t y_offset{ (height - static_cast<std::size_t>(filtered_frame->height)) / 2 };
        for (std::size_t y{ 0 }; std::cmp_less(y, filtered_frame->height); y++) {
            // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            std::memcpy(padded_frame->data[0] + ((y + y_offset) * padded_frame->linesize[0]) + (x_offset * 3),
                        filtered_frame->data[0] + (y * filtered_frame->linesize[0]),
                        static_cast<std::size_t>(filtered_frame->width) * 3);
            // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        }

        return frame_to_png(padded_frame.get());
    }

    return frame_to_png(filtered_frame.get());
}

bool video::VideoProcessor::remux_to_hls(const std::string& out_dir, const std::string& name, int hls_time)
{
    const std::string playlist_path{ std::format("{}/{}.m3u8", out_dir, name) };
    const std::string segment_pattern{ std::format("{}/{}_%03d.ts", out_dir, name) };

    AVOutputFormatContextPtr output_format_context;
    if (const int ret{ avformat_alloc_output_context2(std::inout_ptr(output_format_context), nullptr, "hls", playlist_path.c_str()) }; ret < 0) {
        logging::error{ "Could not create HLS output context: {}", err2str(ret) };
        return false;
    }

    // Map all input streams to output
    const std::span input_streams(_format_context->streams, _format_context->nb_streams);
    for (const AVStream* const in_stream : input_streams) {
        AVStream* const out_stream{ avformat_new_stream(output_format_context.get(), nullptr) };
        if (out_stream == nullptr) {
            logging::error{ "Could not allocate output stream" };
            return false;
        }

        if (const int ret{ avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar) }; ret < 0) {
            logging::error{ "Could not copy codec parameters: {}", err2str(ret) };
            return false;
        }

        out_stream->codecpar->codec_tag = 0;
    }

    // Set HLS muxer options
    AVDictionaryPtr hls_opts{ nullptr };
    av_dict_set_int(std::out_ptr(hls_opts), "hls_time", hls_time, 0);
    av_dict_set_int(std::inout_ptr(hls_opts), "hls_list_size", 0, 0);
    av_dict_set(std::inout_ptr(hls_opts), "hls_flags", "independent_segments", 0);
    av_dict_set(std::inout_ptr(hls_opts), "hls_segment_type", "mpegts", 0);
    av_dict_set(std::inout_ptr(hls_opts), "hls_segment_filename", segment_pattern.c_str(), 0);

    // Custom IO
    output_format_context->flags |= AVFMT_FLAG_CUSTOM_IO;
    output_format_context->io_open = hls_io_open;
    output_format_context->io_close2 = hls_io_close;

    if (const int ret{ avformat_write_header(output_format_context.get(), std::inout_ptr(hls_opts)) }; ret < 0) {
        logging::error{ "Could not write HLS header: {}", err2str(ret) };
        return false;
    }

    const AVPacketPtr packet{ av_packet_alloc() };
    if (packet == nullptr) {
        logging::error{ "Could not allocate packet" };
        return false;
    }

    int ret{ 0 };
    while ((ret = av_read_frame(_format_context.get(), packet.get())) != AVERROR_EOF) {
        if (ret < 0) {
            logging::error{ "Error reading frame: {}", err2str(ret) };
            return false;
        }

        const unsigned int stream_index{ static_cast<unsigned int>(packet->stream_index) };
        if (stream_index >= _format_context->nb_streams) {
            av_packet_unref(packet.get());
            continue;
        }

        const AVStream* const in_stream{ input_streams[stream_index] };

        const std::span output_streams(output_format_context->streams, output_format_context->nb_streams);
        const AVStream* const out_stream{ output_streams[stream_index] };

        av_packet_rescale_ts(packet.get(), in_stream->time_base, out_stream->time_base);
        packet->pos = -1;

        if (const int write_ret{ av_interleaved_write_frame(output_format_context.get(), packet.get()) }; write_ret < 0) {
            logging::error{ "Error writing frame: {}", err2str(write_ret) };
            return false;
        }
    }

    if (const int ret{ av_write_trailer(output_format_context.get()) }; ret < 0) {
        logging::error{ "Could not write trailer: {}", err2str(ret) };
        return false;
    }

    logging::info{ "Conversion complete: {}", playlist_path };
    return true;
}

inline int video::hls_io_write_packet(void* opaque, const std::uint8_t* buf, int buf_size)
{
    auto* ctx{ static_cast<HlsIoContext*>(opaque) };
    ctx->stream.write(reinterpret_cast<const char*>(buf), buf_size); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    return ctx->stream ? buf_size : AVERROR(EIO);
}

inline int video::hls_io_open(AVFormatContext* /*ctx*/, AVIOContext** pb, const char* url, int /*flags*/, AVDictionary** /*options*/)
{
    std::unique_ptr hls_ctx{ std::make_unique<HlsIoContext>() };
    if (hls_ctx == nullptr)
        return AVERROR(ENOMEM);

    hls_ctx->stream.open(url, std::ios::binary | std::ios::trunc);
    if (!hls_ctx->stream.is_open()) {
        logging::error{ "Could not open output file '{}'", url };
        return AVERROR(EIO);
    }

    constexpr std::size_t avio_buffer_size{ 4096 };
    std::uint8_t* const avio_buf{ static_cast<std::uint8_t*>(av_malloc(avio_buffer_size)) };
    if (avio_buf == nullptr) {
        return AVERROR(ENOMEM);
    }

    *pb = avio_alloc_context(avio_buf, avio_buffer_size, 1, hls_ctx.get(), nullptr, hls_io_write_packet, nullptr);
    if (*pb == nullptr) {
        av_free(avio_buf);
        return AVERROR(ENOMEM);
    }

    std::ignore = hls_ctx.release(); // freed in hls_io_close

    logging::debug{ "Opened '{}'", url };
    return 0;
}

inline int video::hls_io_close(AVFormatContext* /*ctx*/, AVIOContext* raw_pb)
{
    const AVIOContextPtr pb(raw_pb);
    if (pb == nullptr) {
        return AVERROR(ENOMEM);
    }

    avio_flush(pb.get());

    const std::unique_ptr<HlsIoContext> hls_ctx(static_cast<HlsIoContext*>(pb->opaque));

    return 0;
}

inline void video::AVOutputFormatDeleter::operator()(AVFormatContext* format_context) const
{
    avformat_free_context(format_context);
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

inline void video::AVDeleter::operator()(AVDictionary* opts) const
{
    av_dict_free(&opts);
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
    std::ranges::generate(raw_pointers, [pos{ 0UZ }, &frame]() mutable -> png_bytep {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        return frame->data[0] + (pos++ * frame->linesize[0]);
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
