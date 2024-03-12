#pragma once

#include "VideoFrame.h"

class IVideoRx
{
public:
    virtual auto connect() -> void = 0;
    virtual auto get_frame_format() -> VideoFrame::Format = 0;
    virtual auto send_control_message() -> void = 0;
    virtual auto recv_frame() -> VideoFramePtr = 0;
};

