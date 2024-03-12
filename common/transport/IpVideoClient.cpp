#include "transport/IpVideoClient.h"
#include "VideoFrame.h"
#include <cstring>
#include <err.h>
#include <arpa/inet.h>
#include <sys/socket.h>

IpVideoClient::IpVideoClient(const std::string &connect_addr, int connect_port):
    _frame_format{nullptr}
{
    _connect_sa.sin_family = AF_INET;
    _connect_sa.sin_addr.s_addr = inet_addr(connect_addr.c_str());
    _connect_sa.sin_port = htons(connect_port);

    _stream_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (_stream_fd == -1)
        err(1, "socket");

    _dgram_fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (_dgram_fd == -1)
        err(1, "socket");
}

auto IpVideoClient::connect() -> void
{
    if (connect(_stream_fd, (sockaddr*)&_connect_sa, sizeof _connect_sa) == -1)
        err(1, "connect");

    uint16_t recv_port = htons(ntohs(_connect_sa.sin_port) + 1);

    sockaddr_in dgram_sa;
    dgram_sa.sin_family = AF_INET;
    dgram_sa.sin_addr.s_addr = inet_addr("0.0.0.0");
    dgram_sa.sin_port = recv_port;

    if (bind(_dgram_fd, (sockaddr*)&dgram_sa, sizeof dgram_sa) == -1)
        err(1, "bind");

    if (send(_stream_fd, &recv_port, sizeof recv_port, 0) != sizeof recv_port)
        err(1, "send");

    VideoFrame::Format frame_format;

    if (recv(_stream_fd, &frame_format, sizeof frame_format, MSG_WAITALL) != sizeof frame_format)
        err(1, "recv");

    frame_format.width = ntohs(frame_format.width);
    frame_format.height = ntohs(frame_format.height);
    frame_format.num_components = ntohs(frame_format.num_components);
    frame_format.bits_per_pixel = ntohs(frame_format.bits_per_pixel);

    _frame_format = std::make_unique<VideoFrame::Format>(frame_format);
}

auto IpVideoClient::send_control_message() -> void
{
    // Unimplemented
}

auto IpVideoClient::recv_frame() -> VideoFramePtr
{
    ssize_t ret;

    ret = recv(_dgram_fd, _frame_buffer.data(), _frame_buffer.size() - 1500, 0);

    if (ret == -1)
        err(1, "recv");

    auto video_frame = std::make_shared<VideoFrame>();
    video_frame->buffer.resize(ret);

    memcpy(video_frame->buffer.data(), _frame_buffer.data(), ret);

    return video_frame;
}

auto IpVideoClient::get_frame_format() -> VideoFrame::Format
{
    if (!_frame_format)
        errx(1, "client not connected");

    return *_frame_format.get();
}

