#pragma once

#include "IVideoSource.h"
#include "libuvc/libuvc.h"
#include <string>
#include <vector>

class UVCVideoSource : public IVideoSource
{
private:
    uvc_context_t *_ctx;
    uvc_device_t *_dev;
    uvc_device_handle_t *_handle;
    const uvc_format_desc_t *_format_descs;
    uvc_stream_ctrl_t _stream_ctrl;

	VideoFrame::Format _video_format;
    ReadFrameHandler _read_frame_handler;

    void shutter();
    void set_mode_radiometric();

public:
    UVCVideoSource();

    auto handle_read_frame(const ReadFrameHandler &handler) -> void override;
    auto get_video_format() -> VideoFrame::Format override;
    auto start() -> bool override;
};

