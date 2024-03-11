#include "IVideoSource.h"
#include "UVCVideoSource.h"

auto open_video_source(VideoSourceType type, const VideoSourceParams &params) -> IVideoSourcePtr
{
    if (type == VideoSourceType::VIDEO_SOURCE_UVC_CAMERA)
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

    return nullptr;
}

