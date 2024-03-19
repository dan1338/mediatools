#pragma once

#include "VideoFrame.h"

class IVideoDisplay
{
public:
    virtual auto open() -> void = 0;
    virtual auto set_video_frame(const VideoFramePtr&) -> void = 0;
    virtual auto update() -> void = 0;
};

extern auto create_glfw_video_display(int width, int height) -> IVideoDisplay*;

