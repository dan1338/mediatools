#pragma once

#include <cstdint>
#include <vector>
#include <memory>

struct VideoFrame
{
	std::vector<uint8_t> buffer;

	enum class Compression
	{
		NONE = 0,
		JPEG_LS,
		JPEG_XL,
	} compression{Compression::NONE};

	struct Format
	{
		uint16_t width;
		uint16_t height;
		uint16_t num_components;
		uint16_t bits_per_pixel;
	};
};

using VideoFramePtr = std::shared_ptr<VideoFrame>;

