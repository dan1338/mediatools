#include "VideoSource/IVideoSource.h"
#include "transport/IpVideoServer.h"
#include "compression/JpegLs.h"
#include "FramePipeline.h"
#include <cstring>
#include <getopt.h>
#include <cstdio>
#include <err.h>

struct VideoStremerContext
{
    IVideoSourcePtr video_source;
    IVideoTxPtr video_tx;
    JpegLsEncoder jpeg_encoder;
    FramePipeline<VideoFrame> pre_tx_pipeline;
};

VideoStremerContext create_context(int argc, char **argv)
{
    VideoStremerContext ret;

	bool use_usbdev{true};
	std::string recording_path{""};
	std::string listen_addr{"0.0.0.0"};
	std::string listen_port{"9000"};

	int ch;
	while (ch = getopt(argc, argv, "l:f:"), ch != -1)
	{
		switch (ch)
		{
		case 'l':
			if (auto delim = strcspn(optarg, ":"); delim != strlen(optarg))
			{
				if (auto addr = std::string{optarg}.substr(0, delim); addr.size())
				{
					listen_addr = addr;
				}
				if (auto port = std::string{optarg}.substr(delim + 1); port.size())
				{
					listen_port = port;
				}
			}

			break;
		case 'f':
			recording_path = optarg;
			use_usbdev = false;

			break;
		case '?':
			errx(1, "usage: %s [-l [addr]:port] [-f file]", *argv);
		}
	}

    ret.video_tx = std::make_unique<IpVideoServer>(listen_addr, std::stoi(listen_port));

	if (!recording_path.empty())
	{
		ret.video_source = open_video_source(VideoSourceType::FILE_SEQ, {{"path", recording_path}, {"fps", "24"}});
	}
	else
	{
		ret.video_source = open_video_source(VideoSourceType::UVC_CAMERA);
	}

    ret.pre_tx_pipeline.add_component(&ret.jpeg_encoder);

    return ret;
}

int main(int argc, char **argv)
{
    auto ctx = create_context(argc, argv);

    const auto frame_format = ctx.video_source->get_video_format();

    ctx.jpeg_encoder.set_frame_format(frame_format);
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

        const auto processed_frame = ctx.pre_tx_pipeline.process_frame(frame);
        ctx.video_tx->send_frame(processed_frame);
    });

    ctx.video_source->start();

    while (1)
    {
        ctx.video_tx->poll_client();
    }

    return 0;
}

