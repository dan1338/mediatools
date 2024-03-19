#pragma once
#include "VideoFrame.h"
#include "compression/JpegLs.h"
#include "FramePipeline.h"
#include <filesystem>
#include <vector>
#include <string>


class VideoSequenceReader
{
public:
    VideoSequenceReader(const std::string &path);
    auto read_frame() -> VideoFramePtr;

private:
    std::string _path;
    std::vector<std::filesystem::path> _files;
    size_t _read_idx;
    JpegLsDecoder _decoder;
};
