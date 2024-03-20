#include "IVideoSource.h"
#include "UVCVideoSource.h"
#include "RecordingVideoSource.h"

auto open_video_source(VideoSourceType type, const VideoSourceParams &params) -> IVideoSourcePtr
{
	if (type == VideoSourceType::UVC_CAMERA)
	{
		if (auto it = params.find("id"); it != params.end())
		{
			// NOT IMPLEMENTED
			// return std::make_unique<UVCVideoSource>(std::stoi(it->second));
		}
		else
		{
			return std::make_unique<UVCVideoSource>();
		}
	}
	else if (type == VideoSourceType::FILE_SEQ)
	{
		auto path = params.at("path");
		auto fps = std::stoi(params.at("fps"));

		return std::make_unique<RecordingVideoSource>(path, fps);
	}

	return nullptr;
}

