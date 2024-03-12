#pragma once

#include "VideoFrame.h"
#include <memory>

class IVideoRx
{
public:
    virtual auto connect() -> void = 0;
    virtual auto get_frame_format() -> VideoFrame::Format = 0;
    virtual auto send_control_message() -> void = 0;
    virtual auto recv_frame() -> VideoFramePtr = 0;
};

using IVideoRxPtr = std::unique_ptr<IVideoRx>;
