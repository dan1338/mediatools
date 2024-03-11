#include "VideoSource/IVideoSource.h"
#include "Server.h"
#include <cstdio>

int main(int argc, char **argv)
{
    auto video_source = open_video_source(VideoSourceType::VIDEO_SOURCE_UVC_CAMERA);
    const auto video_format = video_source->get_video_format();

    VideoServer server("0.0.0.0", 9000, video_format);
    server.await_connection();

    video_source->handle_read_frame([&](VideoFramePtr frame) {
        printf("uvc_read_frame (%zu bytes)\n", frame->buffer.size());

        size_t imgdata_size = video_format.width * video_format.height * 2;

        if (imgdata_size < frame->buffer.size())
        {
            printf("[!] incomplete frame\n");
            return;
        }

        server.send_frame(frame->buffer.data(), imgdata_size);
    });

    video_source->start();

    return 0;
}

