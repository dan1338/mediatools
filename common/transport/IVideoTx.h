#pragma once

#include <functional>
#include <memory>
#include "VideoFrame.h"

class IVideoTx
{
public:
    using ControlMessageHandler = std::function<void()>;

    virtual auto set_frame_format(VideoFrame::Format format) -> void = 0;

    virtual auto handle_control_message(const ControlMessageHandler &handler) -> void = 0;
    virtual auto await_connection() -> void = 0;
    virtual auto poll_client() -> void = 0;

    virtual auto send_frame(const VideoFramePtr &frame) -> void = 0;
};

using IVideoTxPtr = std::unique_ptr<IVideoTx>;
