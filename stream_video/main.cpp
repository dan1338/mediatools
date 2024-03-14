#include "VideoSource/IVideoSource.h"
#include "transport/IpVideoServer.h"
#include <cstdio>
#include <err.h>

struct VideoStremerContext
{
    IVideoSourcePtr video_source;
    IVideoTxPtr video_tx;
};

VideoStremerContext create_context(int argc, char **argv)
{
    VideoStremerContext ret;

    if (argc < 3)
        errx(1, "usage: [listen_addr] [listen_port]");

    const auto listen_addr = argv[1];
    const auto listen_port = std::stoi(argv[2]);

    ret.video_tx = std::make_unique<IpVideoServer>(listen_addr, listen_port);
    ret.video_source = open_video_source(VideoSourceType::VIDEO_SOURCE_UVC_CAMERA);

    return ret;
}

int main(int argc, char **argv)
{
    auto ctx = create_context(argc, argv);

    const auto frame_format = ctx.video_source->get_video_format();

    ctx.video_tx->set_frame_format(frame_format);
    ctx.video_tx->await_connection();

    ctx.video_source->handle_read_frame([&](VideoFramePtr frame) {
        printf("uvc_read_frame (%zu bytes)\n", frame->buffer.size());

        size_t imgdata_size = frame_format.width * frame_format.height * 2;

        if (frame->buffer.size() < imgdata_size)
        {
            printf("[!] incomplete frame\n");
            return;
        }

        ctx.video_tx->send_frame(frame);
    });

    ctx.video_source->start();

    while (1)
    {
        ctx.video_tx->poll_client();
    }

    return 0;
}

