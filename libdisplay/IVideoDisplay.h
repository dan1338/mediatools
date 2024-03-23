#pragma once

#include "VideoFrame.h"

class IVideoDisplay
{
public:
    virtual auto open() -> void = 0;
    virtual auto set_video_frame(const VideoFramePtr&) -> void = 0;
    virtual auto update() -> bool = 0;
};

extern auto create_glfw_video_display(int width, int height) -> IVideoDisplay*;

#ifdef __ANDROID__
#include <android/native_window.h>
extern auto create_android_video_display(ANativeWindow *awindow) -> IVideoDisplay*;
extern auto create_null_video_display() -> IVideoDisplay*;
#endif
