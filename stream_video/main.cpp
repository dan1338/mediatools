#include "video_source/IVideoSource.h"
#include <cstdio>

int main(int argc, char **argv)
{
    auto video_source = open_video_source(VideoSourceType::VIDEO_SOURCE_UVC_CAMERA);
    const auto video_format = video_source->get_video_format();

    video_source->handle_read_frame([&](VideoFrame frame) {
        printf("uvc_read_frame (%u bytes)\n", frame.data_size);

        size_t imgdata_size = video_format.width * video_format.height * 2;

        if (imgdata_size < frame.data_size)
        {
            printf("[!] incomplete frame\n");
            return;
        }
    });

    video_source->start();

    return 0;
}

