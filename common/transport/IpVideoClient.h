#pragma once

#include "VideoFrame.h"
#include "transport/IVideoRx.h"
#include <arpa/inet.h>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <sys/socket.h>

class IpVideoClient : public IVideoRx
{
public:
    IpVideoClient(const std::string &connect_addr, int connect_port);

    auto connect() -> void override;
    auto get_frame_format() -> VideoFrame::Format override;
    auto send_control_message() -> void override;
    auto recv_frame() -> VideoFramePtr override;

private:
    std::unique_ptr<VideoFrame::Format> _frame_format;
    std::array<uint8_t, 65535> _frame_buffer;

    sockaddr_in _connect_sa;

    int _stream_fd;
    int _dgram_fd;
};

