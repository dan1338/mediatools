#pragma once
#include "storage/VideoSequenceReader.h"
#include "IVideoSource.h"
#include <thread>

class RecordingVideoSource : public IVideoSource
{
public:
	RecordingVideoSource(const std::string &path, int fps);

    auto handle_read_frame(const ReadFrameHandler &handler) -> void override;
    auto get_video_format() -> VideoFrame::Format override;
    auto start() -> bool override;
private:
	int _fps;
	VideoSequenceReader _seq_reader;
	ReadFrameHandler _handler;
	std::unique_ptr<std::thread> _thread;
};
