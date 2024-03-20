#include "VideoSequenceReader.h"
#include <algorithm>
#include <filesystem>
#include <fstream>

VideoSequenceReader::VideoSequenceReader(const std::string &path):
    _path(path),
    _read_idx(0)
{
    for (const auto &ent : std::filesystem::directory_iterator(path))
    {
        _files.push_back(ent.path());
    }

    std::sort(_files.begin(), _files.end(), [](auto &lhs, auto &rhs){
        return std::stoi(lhs.stem()) < std::stoi(rhs.stem());
    });
}

auto VideoSequenceReader::rewind() -> void
{
	_read_idx = 0;
}

auto VideoSequenceReader::read_frame() -> VideoFramePtr
{
    if (_read_idx >= _files.size())
        return nullptr;

    auto frame = std::make_shared<VideoFrame>();
    frame->compression = VideoFrame::Compression::JPEG_LS;

    const auto file_path = _files[_read_idx++];

    size_t file_size = std::filesystem::file_size(file_path);
    frame->buffer.resize(file_size);

    std::ifstream fp(file_path, std::ios::binary);
    fp.read((char*)frame->buffer.data(), file_size);

    return _decoder.process_frame(frame);
}

