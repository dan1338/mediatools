#pragma once

#include "VideoFrame.h"

class IVideoDisplay
{
public:
    virtual auto open() -> void = 0;
    virtual auto set_video_frame(const VideoFramePtr&) -> void = 0;
    virtual auto update() -> bool = 0;
};

class IWindow
{
public:
    virtual auto resize(int width, int height) -> void = 0;
    virtual auto poll() -> void = 0;
    virtual auto should_close() -> bool = 0;
    virtual auto swap_buffers() -> void = 0;
    virtual auto get_size() -> std::pair<int, int> = 0;
};

class NullWindow : public IWindow
{
public:
    auto resize(int width, int height) -> void
    {
        _w = width;
        _h = height;
    }
    auto poll() -> void {}
    auto should_close() -> bool {return false;}
    auto swap_buffers() -> void {}

    auto get_size() -> std::pair<int, int>
    {
        return {_w, _h};
    }

private:
    int _w, _h;
};

extern auto create_glfw_video_display(int width, int height) -> IVideoDisplay*;
extern auto create_user_window_video_display(IWindow *window) -> IVideoDisplay*;
