#include "UVCVideoSource.h"
#include "libuvc/libuvc.h"
#include <err.h>

#define UVC_SHUTTER 0x8000
#define UVC_MODE_RADIOMETRIC 0x8004

UVCVideoSource::UVCVideoSource()
{
    uvc_error_t err;

    if (err = uvc_init(&_ctx, 0); err != UVC_SUCCESS)
        errx(1, "uvc_init (%s)", uvc_strerror(err));

    int vid{0}, pid{0};
    const char *sn = nullptr;

    if (err = uvc_find_device(_ctx, &_dev, vid, pid, sn); err != UVC_SUCCESS)
        errx(1, "uvc_find_device (%s)", uvc_strerror(err));

    if (err = uvc_open(_dev, &_handle); err != UVC_SUCCESS)
        errx(1, "uvc_open (%s)", uvc_strerror(err));

    _format_descs = uvc_get_format_descs(_handle);

    const auto *frame_desc = &_format_descs->frame_descs[0];

    if (err = uvc_get_stream_ctrl_format_size(
                _handle, &_stream_ctrl, UVC_FRAME_FORMAT_ANY,
                frame_desc->wWidth, frame_desc->wHeight, 0); err != UVC_SUCCESS)
        errx(1, "uvc_get_stream_ctrl_format_size (%s)", uvc_strerror(err));

    set_mode_radiometric();
    shutter();

    _video_format.width = frame_desc->wWidth;
    _video_format.height = frame_desc->wHeight;
    _video_format.num_channels = 1;
    _video_format.bits_per_pixels = 16;
}

void UVCVideoSource::shutter()
{
    uvc_set_zoom_abs(_handle, UVC_SHUTTER);
}

void UVCVideoSource::set_mode_radiometric()
{
    uvc_set_zoom_abs(_handle, UVC_MODE_RADIOMETRIC);
}

void UVCVideoSource::handle_read_frame(const ReadFrameHandler &handler)
{
    _read_frame_handler = handler;
}

VideoFormat UVCVideoSource::get_video_format()
{
    return _video_format;
}

void frame_callback(uvc_frame *uvc_frame, void *user_ptr)
{
    VideoFrame video_frame;
    video_frame.data = (uint8_t*)uvc_frame->data;
    video_frame.data_size = uvc_frame->data_bytes;

    auto &handler = *(IVideoSource::ReadFrameHandler*)user_ptr;
    handler(video_frame);
}

bool UVCVideoSource::start()
{
    uvc_error_t err;

    if (err = uvc_start_streaming(_handle, &_stream_ctrl, frame_callback, &_read_frame_handler, 0); err != UVC_SUCCESS)

    {
        warnx("uvc_start_streaming (%s)", uvc_strerror(err));
        return false;
    }

    return true;
}

