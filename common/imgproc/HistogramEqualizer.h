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

    auto stretch_historgram(const VideoFramePtr &frame) -> VideoFramePtr
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

        uint16_t vmin = lo_bin * bin_size, vmax = ((hi_bin + 1) * bin_size - 1);
        uint16_t vspan = vmax - vmin;

        for (size_t i = 0; i < w * h; i++)
        {
            uint16_t &v = ptr[i];

            if (v >= vmin) {
                v -= vmin;
                v = 65535 * v / vspan;
            } else {
                v = 0;
            }
        }

        return frame;
    }

    auto process_frame(const VideoFramePtr &frame) -> VideoFramePtr
    {
        uint32_t w = frame->format.width, h = frame->format.height;
        uint32_t n = w * h;

        uint16_t *ptr = (uint16_t*)frame->buffer.data();

        std::array<uint32_t, NBins> hist = {0};
        uint32_t bin_size = 65536 / NBins;

        for (size_t i = 0; i < w * h; i++)
        {
            uint16_t &v = ptr[i];
            hist[v / bin_size] += 1;
        }

        std::array<uint32_t, NBins> cdf;
        uint32_t cdf_min{0};
        cdf[0] = hist[0];

        for (size_t i = 1; i < hist.size(); i++) {
            cdf[i] = cdf[i - 1] + hist[i];

            if (cdf[i] > 0 && cdf_min == 0)
                cdf_min = cdf[i];
        }

        for (size_t i = 0; i < n; i++)
        {
            uint16_t &v = ptr[i];
            uint32_t num = cdf[v / bin_size] - cdf_min;
            uint32_t den = n - cdf_min;
            v = round((num / (float)den) * 65535);
        }

        return frame;
    }
};
