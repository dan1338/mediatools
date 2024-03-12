#pragma once

#include "FramePipeline.h"
#include "VideoFrame.h"
#include <memory>

class JpegLsEncoder : public FramePipeline<VideoFrame>::IComponent
{
public:
    std::shared_ptr<VideoFrame> process_frame(const std::shared_ptr<VideoFrame> &frame) override;
};

