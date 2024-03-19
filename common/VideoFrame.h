#pragma once

#include <cstdint>
#include <vector>
#include <memory>

struct VideoFrame
{
	std::vector<uint8_t> buffer;

	struct Format
	{
		uint16_t width;
		uint16_t height;
		uint16_t num_components;
		uint16_t bits_per_pixel;
	} format;

	enum class Compression
	{
		NONE = 0,
		JPEG_LS,
		JPEG_XL,
	} compression{Compression::NONE};
};

using VideoFramePtr = std::shared_ptr<VideoFrame>;

