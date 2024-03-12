#include "transport/IpVideoServer.h"
#include <err.h>
#include <sys/socket.h>

IpVideoServer::IpVideoServer(const std::string &listen_addr, int listen_port):
    _frame_format{nullptr}, _control_message_handler{nullptr}
{
    _listen_sa.sin_family = AF_INET;
    _listen_sa.sin_addr.s_addr = inet_addr(listen_addr.c_str());
    _listen_sa.sin_port = htons(listen_port);

    _listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (_listen_fd == -1)
        err(1, "socket");

    _dgram_fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (_dgram_fd == -1)
        err(1, "socket");
}

auto IpVideoServer::set_frame_format(VideoFrame::Format format) -> void
{
    _frame_format = std::make_unique<VideoFrame::Format>(format);
    _frame_format->width = htons(_frame_format->width);
    _frame_format->height = htons(_frame_format->height);
    _frame_format->num_components = htons(_frame_format->num_components);
    _frame_format->bits_per_pixel = htons(_frame_format->bits_per_pixel);
}

auto IpVideoServer::handle_control_message(const ControlMessageHandler &handler) -> void
{
    _control_message_handler = std::make_unique<ControlMessageHandler>(handler);
}

auto IpVideoServer::await_connection() -> void
{
    int ret;
    int reuse_addr = 1;

    setsockopt(_listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof reuse_addr);

    if (ret = bind(_listen_fd, (sockaddr*)&_listen_sa, sizeof _listen_sa); ret == -1)
        err(1, "bind");

    if (listen(_listen_fd, 1) == -1)
        err(1, "listen");

    socklen_t socklen = sizeof _client_sa;

    if (_stream_fd = accept(_listen_fd, (sockaddr*)&_client_sa, &socklen); _stream_fd == -1)
        err(1, "accept");

    uint16_t reply_port;
    recv(_stream_fd, &reply_port, sizeof reply_port, MSG_WAITALL);

    _client_sa.sin_port = ntohs(reply_port);

    if (!_frame_format)
        errx(1, "no frame format set");

    send(_stream_fd, _frame_format.get(), sizeof(VideoFrame::Format), 0);
}

auto IpVideoServer::poll_client() -> void
{
    char tmp;
    recv(_stream_fd, &tmp, sizeof tmp, MSG_PEEK);
}

auto IpVideoServer::send_frame(const VideoFramePtr &frame) -> void
{
    ssize_t ret;

    ret = sendto(_dgram_fd, frame->buffer.data(), frame->buffer.size(), 0,
            (sockaddr*)&_client_sa, sizeof _client_sa);

    if (ret != frame->buffer.size())
    {
        warn("send (incomplete)");
    }
}

