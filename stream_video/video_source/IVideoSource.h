#pragma once

#include <functional>
#include <memory>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

struct VideoFormat
{
    uint16_t width;
    uint16_t height;
    uint8_t num_channels;
    uint8_t bits_per_pixels;
};

struct VideoFrame
{
    uint8_t *data;
    uint32_t data_size;
};

enum class VideoSourceType
{
    VIDEO_SOURCE_UVC_CAMERA,
};

class IVideoSource
{
public:
    using ReadFrameHandler = std::function<void(VideoFrame)>;
    virtual void handle_read_frame(const ReadFrameHandler &handler) = 0;
    virtual VideoFormat get_video_format() = 0;
    virtual bool start() = 0;
};

using IVideoSourcePtr = std::unique_ptr<IVideoSource>;
using VideoSourceParams = std::map<std::string, std::string>;

IVideoSourcePtr open_video_source(VideoSourceType type, const VideoSourceParams &params = {});

