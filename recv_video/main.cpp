#include "transport/IVideoRx.h"
#include "transport/IpVideoClient.h"
#include "compression/JpegLs.h"
#include "storage/VideoSequenceWriter.h"
#include "FramePipeline.h"
#include "IVideoDisplay.h"
#include "opencv2/opencv.hpp"
#include <cmath>
#include <cstdio>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <sys/stat.h>
#include <ctime>
#include <err.h>
#include <memory>
#include <string>

class HistogramEqualizer : public FramePipeline<VideoFrame>::IComponent
{
public:
	HistogramEqualizer()
	{
	}

	auto process_frame(const VideoFramePtr &frame) -> VideoFramePtr
	{
		cv::Size img_size(frame->format.width, frame->format.height - 4);
		cv::Mat img(img_size, CV_16UC1, frame->buffer.data());

        std::array<uint32_t, 256> hist = {0};

		for (size_t i = 0; i < img.size[0]*img.size[1]; i++)
		{
			uint16_t &v = img.at<uint16_t>(i);
            hist[v / 256] += 1;
		}

        uint32_t lo_thresh = 1000, hi_thresh = 200;
        int lo_bin{-1}, hi_bin{-1};

        for (size_t i = 0; i < hist.size(); i++)
        {
            if (lo_bin == -1 && hist[i] > lo_thresh)
            {
                lo_bin = i;
                hi_bin = i;
            }
            else if (hist[i] > hi_thresh)
            {
                hi_bin = i;
            }
        }

        printf("hist: lo(%d) hi(%d) / %d - %d\n", lo_bin, hi_bin, lo_bin*256, (hi_bin + 1)*256);

        uint16_t vmin = lo_bin * 256, vmax = ((hi_bin + 1) * 256 - 1);
        uint16_t vspan = vmax - vmin;

		for (size_t i = 0; i < img.size[0]*img.size[1]; i++)
		{
			uint16_t &v = img.at<uint16_t>(i);

            v -= vmin;
            v = 65535 * v / vspan;

			//v = std::sqrt(v / 65535.0f) / 4;
			//v = std::min(65535, std::max(0, v - 1500) * 65535 / 10000);
		}


		return frame;
	}
};

struct VideoRecieverContext
{
    IVideoRxPtr video_rx;
    FramePipeline<VideoFrame> rx_pipeline;
};

VideoRecieverContext create_context(int argc, char **argv)
{
    VideoRecieverContext ret;

    if (argc < 3)
        errx(1, "usage: [connect_addr] [connect_port]");

    const auto connect_addr = argv[1];
    const auto connect_port = std::stoi(argv[2]);

    ret.video_rx = std::make_unique<IpVideoClient>(connect_addr, connect_port);
    ret.rx_pipeline.make_component<JpegLsDecoder>();
    ret.rx_pipeline.make_component<HistogramEqualizer>();
    ret.rx_pipeline.make_component<VideoSequenceWriter>("OUT");

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

	auto display = create_glfw_video_display(1280, 960);
	display->open();

	while (1)
	{
		auto frame = ctx.rx_pipeline.process_frame(ctx.video_rx->recv_frame());

		display->set_video_frame(frame);

		if (!display->update())
			break;
	}

    return 0;
}

