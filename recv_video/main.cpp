#include "transport/IVideoRx.h"
#include "transport/IpVideoClient.h"
#include "compression/JpegLs.h"
#include "FramePipeline.h"
#include "opencv2/opencv.hpp"
#include <cmath>
#include <cstdio>
#include <opencv2/highgui.hpp>
#include <sys/stat.h>
#include <ctime>
#include <err.h>
#include <memory>
#include <string>

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

    return ret;
}

class FrameWriter
{
private:
	std::string _base_dir;
	uint32_t _frame_idx;

	std::string make_base_dir()
	{
		time_t t = time(0);
		const auto tm = gmtime(&t);

		std::string ret;

		ret += std::to_string(tm->tm_year + 1900);
		ret += std::to_string(tm->tm_mon + 1);
		ret += std::to_string(tm->tm_mday + 1);
		ret += "_";
		ret += std::to_string(tm->tm_hour + 1);
		ret += std::to_string(tm->tm_min + 1);
		ret += std::to_string(tm->tm_sec + 1);

		return ret;
	}

public:
	FrameWriter(): FrameWriter(make_base_dir())
	{
	}

	FrameWriter(const std::string &base_dir): _base_dir(base_dir), _frame_idx(0)
	{
		mkdir(base_dir.c_str(), 0755);
	}

	void write_frame(const VideoFramePtr &frame)
	{
		std::string path{_base_dir};

		path += "/";
		path += std::to_string(_frame_idx);

		FILE *fp = fopen(path.c_str(), "wb");
		fwrite(frame->buffer.data(), 1, frame->buffer.size(), fp);
		fclose(fp);

		++_frame_idx;
	}
};

int main(int argc, char **argv)
{
    auto ctx = create_context(argc, argv);

    ctx.video_rx->connect();
    const auto frame_format = ctx.video_rx->get_frame_format();

    printf("video format: (%dx%d) (%d channel) (%d bpp)\n",
            frame_format.width, frame_format.height,
            frame_format.num_components, frame_format.bits_per_pixel);

	FrameWriter writer("out");

	cv::Size img_size(frame_format.width, frame_format.height);

	while (1)
	{
        const auto frame = ctx.rx_pipeline.process_frame(ctx.video_rx->recv_frame());

		//writer.write_frame(frame);

		cv::Mat img(img_size, CV_16UC1, frame->buffer.data());

        uint16_t vmin = 65535, vmax = 0;

		for (size_t i = 0; i < img.size[0]*img.size[1]; i++)
		{
			uint16_t &v = img.at<uint16_t>(i);
			//v = std::sqrt(v / 65535.0f) / 4;
			v = std::min(65535, std::max(0, v - 1500) * 65535 / 10000);

            vmin = std::min(v, vmin);
            vmax = std::max(v, vmax);
		}

        printf("vmin(%d) vmax(%d)\n", vmin, vmax);

		cv::imshow("img", img);
        cv::resizeWindow("img", 256 * 3, 192 * 3);
		cv::waitKey(1);
	}

    return 0;
}

