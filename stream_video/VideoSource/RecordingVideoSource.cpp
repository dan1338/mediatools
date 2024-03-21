#include "RecordingVideoSource.h"
#include <chrono>

RecordingVideoSource::RecordingVideoSource(const std::string &path, int fps):
	_fps(fps),
	_seq_reader(path)
{
}

auto RecordingVideoSource::handle_read_frame(const ReadFrameHandler &handler) -> void
{
	_handler = handler;
}

auto RecordingVideoSource::get_video_format() -> VideoFrame::Format
{
	auto frame = _seq_reader.read_frame();
	_seq_reader.rewind();

	return frame->format;
}

auto RecordingVideoSource::start() -> bool
{
	_thread = std::make_unique<std::thread>([&](){
		int us_delta = 1000000 / _fps;

		std::chrono::system_clock clk;
		auto send_time = std::chrono::time_point_cast<std::chrono::microseconds>(clk.now());

		while (1)
		{
			auto frame = _seq_reader.read_frame();
			
			if (frame == nullptr)
				break;

			while (1)
			{
				auto now = std::chrono::time_point_cast<std::chrono::microseconds>(clk.now());

				if ((now - send_time).count() > us_delta)
				{
					send_time = now;
					_handler(frame);
					break;
				}
			}
		}
	});

	return true;
}

