#include "VideoSequenceWriter.h"
#include <algorithm>
#include <filesystem>
#include <fstream>

VideoSequenceWriter::VideoSequenceWriter(const std::string &path):
    _path(path),
    _write_idx(0),
    _initialized(false)
{
    std::error_code err; // ignored
    std::filesystem::create_directory(_path, err);
}

auto VideoSequenceWriter::write_frame(const VideoFramePtr& frame) -> void
{
    if (!_initialized)
    {
        _encoder.set_frame_format(frame->format);
        _initialized = true;
    }

    const auto compressed_frame = _encoder.process_frame(frame);

    const auto file_path = _path / std::to_string(_write_idx++);

    std::ofstream fp(file_path, std::ios::binary);
    fp.write((char*)compressed_frame->buffer.data(), compressed_frame->buffer.size());
}

auto VideoSequenceWriter::process_frame(const std::shared_ptr<VideoFrame>& frame) -> std::shared_ptr<VideoFrame>
{
    write_frame(frame);

    return frame;
}

