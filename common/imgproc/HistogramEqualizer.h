#pragma once

#include "FramePipeline.h"
#include "VideoFrame.h"
#include <array>
#include <cstdint>
#include "log.h"

template<size_t NBins>
class HistogramEqualizer : public FramePipeline<VideoFrame>::IComponent
{
public:
    HistogramEqualizer()
    {
    }

    auto process_frame(const VideoFramePtr &frame) -> VideoFramePtr
    {
        uint32_t w = frame->format.width, h = frame->format.height;
        uint16_t *ptr = (uint16_t*)frame->buffer.data();

        std::array<uint32_t, NBins> hist = {0};
        uint32_t bin_size = 65536 / NBins;

        for (size_t i = 0; i < w * h; i++)
        {
            uint16_t &v = ptr[i];
            hist[v / bin_size] += 1;
        }

        uint32_t lo_thresh = 1000, hi_thresh = 200;
        int lo_bin{-1}, hi_bin{-1};

        for (size_t i = 0; i < hist.size(); i++)
        {
            if (lo_bin == -1 && hist[i] > lo_thresh)
            {
                lo_bin = i;
                hi_bin = i;
            }
            else if (hist[i] > hi_thresh)
            {
                hi_bin = i;
            }
        }

        DEBUG("hist: lo(%d) hi(%d) / %d - %d\n", lo_bin, hi_bin, lo_bin*bin_size, (hi_bin + 1)*bin_size);

        uint16_t vmin = lo_bin * bin_size, vmax = ((hi_bin + 1) * bin_size - 1);
        uint16_t vspan = vmax - vmin;

        for (size_t i = 0; i < w * h; i++)
        {
            uint16_t &v = ptr[i];

            v -= vmin;
            v = 65535 * v / vspan;
        }

        return frame;
    }
};
