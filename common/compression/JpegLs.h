#pragma once

#include "FramePipeline.h"
#include "VideoFrame.h"
#include <cstdint>
#include <memory>

#include "charls/charls_jpegls_encoder.h"
#include "charls/charls_jpegls_decoder.h"

class JpegLsEncoder : public FramePipeline<VideoFrame>::IComponent
{
public:
    auto process_frame(const std::shared_ptr<VideoFrame> &frame) -> std::shared_ptr<VideoFrame> override;
    auto set_frame_format(const VideoFrame::Format &format) -> void;

private:
    charls::jpegls_encoder _jpegls_encoder;
    VideoFrame::Format _frame_format;
    std::vector<uint8_t> _dest_buffer;
};

class JpegLsDecoder : public FramePipeline<VideoFrame>::IComponent
{
public:
    auto process_frame(const std::shared_ptr<VideoFrame> &frame) -> std::shared_ptr<VideoFrame> override;

private:
    charls::jpegls_decoder _jpegls_decoder;
    VideoFrame::Format _frame_format;
};

