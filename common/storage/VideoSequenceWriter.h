#pragma once
#include "VideoFrame.h"
#include "compression/JpegLs.h"
#include "FramePipeline.h"
#include <filesystem>
#include <vector>
#include <string>

class VideoSequenceWriter : public FramePipeline<VideoFrame>::IComponent
{
public:
    VideoSequenceWriter(const std::string &path);
    auto write_frame(const VideoFramePtr& frame) -> void;
    auto process_frame(const std::shared_ptr<VideoFrame>& frame) -> std::shared_ptr<VideoFrame> override;

private:
    std::filesystem::path _path;
    size_t _write_idx;
    bool _initialized;
    JpegLsEncoder _encoder;
};
