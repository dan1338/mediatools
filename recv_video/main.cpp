#include "transport/IVideoRx.h"
#include "transport/IpVideoClient.h"
#include "compression/JpegLs.h"
#include "storage/VideoSequenceWriter.h"
#include "FramePipeline.h"
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
    ret.rx_pipeline.make_component<VideoSequenceWriter>("TEST_VIDEO");

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
        cv::Mat big_img;

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

        cv::rotate(img, img, cv::ROTATE_90_CLOCKWISE);
        cv::resize(img, big_img, cv::Size(256*3, 196*3));
		cv::imshow("img", big_img);
		cv::waitKey(1);
	}

    return 0;
}

