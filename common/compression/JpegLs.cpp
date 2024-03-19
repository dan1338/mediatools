#include "JpegLs.h"

std::shared_ptr<VideoFrame> JpegLsEncoder::process_frame(const std::shared_ptr<VideoFrame> &frame)
{
    _jpegls_encoder.rewind();

    size_t out_size = _jpegls_encoder.encode(frame->buffer.data(), frame->buffer.size());

    return std::make_shared<VideoFrame>(VideoFrame{
        {_dest_buffer.data(), _dest_buffer.data() + out_size},
        frame->format,
        VideoFrame::Compression::JPEG_LS
    });
}

auto JpegLsEncoder::set_frame_format(const VideoFrame::Format &format) -> void
{
    _frame_format = format;

    charls::frame_info frame_info;
    frame_info.width = format.width;
    frame_info.height = format.height;
    frame_info.component_count = format.num_components;
    frame_info.bits_per_sample = format.bits_per_pixel;

    _jpegls_encoder.frame_info(frame_info);

    _dest_buffer.resize(_jpegls_encoder.estimated_destination_size());
    _jpegls_encoder.destination(_dest_buffer);
}

std::shared_ptr<VideoFrame> JpegLsDecoder::process_frame(const std::shared_ptr<VideoFrame> &frame)
{
    std::vector<uint8_t> out_buffer;

    const auto [frame_info, inverleave_mode] = _jpegls_decoder.decode(frame->buffer, out_buffer);

    _frame_format.width = frame_info.width;
    _frame_format.height = frame_info.height;
    _frame_format.num_components = frame_info.component_count;
    _frame_format.bits_per_pixel = frame_info.bits_per_sample;

    return std::make_shared<VideoFrame>(VideoFrame{out_buffer, _frame_format});
}

