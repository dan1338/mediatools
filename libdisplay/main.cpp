#include "IVideoDisplay.h"
#include "VideoFrame.h"
#include "storage/VideoSequenceReader.h"
#include <cstdint>
#include <unistd.h>
#include <vector>

int main(void)
{
    VideoSequenceReader reader("TEST_VIDEO");

    auto disp = create_glfw_video_display(1280, 960);

    disp->open();

    while (1)
    {
        auto img = reader.read_frame();

        if (img == nullptr)
            break;

        disp->set_video_frame(img);

        if (!disp->update())
            break;

        usleep(1000000/24);
    }

    return 0;
}

