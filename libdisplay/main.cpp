#include "IVideoDisplay.h"

#include <vector>

int main(void)
{
    auto disp = create_glfw_video_display(1280, 900);

    disp->open();

    while (disp->)
    {
        disp->update();
    }
}

