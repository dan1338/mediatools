#pragma once

#include <functional>
#include <memory>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "VideoFrame.h"

enum class VideoSourceType
{
    VIDEO_SOURCE_UVC_CAMERA,
};

class IVideoSource
{
public:
    using ReadFrameHandler = std::function<void(VideoFramePtr)>;

    virtual auto handle_read_frame(const ReadFrameHandler &handler) -> void = 0;
    virtual auto get_video_format() -> VideoFrame::Format = 0;
    virtual auto start() -> bool = 0;
};

using IVideoSourcePtr = std::unique_ptr<IVideoSource>;
using VideoSourceParams = std::map<std::string, std::string>;

auto open_video_source(VideoSourceType type, const VideoSourceParams &params = {}) -> IVideoSourcePtr;

