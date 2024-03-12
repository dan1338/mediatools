#include "transport/IVideoRx.h"
#include "transport/IpVideoClient.h"
#include <err.h>
#include <memory>

struct VideoRecieverContext
{
    IVideoRxPtr video_rx;
};

VideoRecieverContext create_context(int argc, char **argv)
{
    VideoRecieverContext ret;

    if (argc < 3)
        errx(1, "usage: [connect_addr] [connect_port]");

    const auto connect_addr = argv[1];
    const auto connect_port = std::stoi(argv[2]);

    ret.video_rx = std::make_unique<IpVideoClient>(connect_addr, connect_port);

    return ret;
}

int main(int argc, char **argv)
{
    auto ctx = create_context(argc, argv);

    ctx.video_rx->connect();
    const auto frame_format = ctx.video_rx->get_frame_format();

    printf("video format: (%dx%d) (%d channel) (%d bpp)\n",
            frame_format.width, frame_format.height,
            frame_format.num_components, frame_format.bits_per_pixel);

    return 0;
}

