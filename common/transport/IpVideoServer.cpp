#include "transport/IpVideoServer.h"
#include <algorithm>
#include <cstring>
#include <err.h>
#include <sys/socket.h>

IpVideoServer::IpVideoServer(const std::string &listen_addr, int listen_port):
    _frame_format{nullptr}, _control_message_handler{nullptr}, _frame_id{0}
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
    constexpr size_t max_msg_size = 65535 - 2000;

    size_t current_pos = 0;
    size_t bytes_remaining = frame->buffer.size();

    uint8_t frag_id = 0;
    uint8_t num_fragments = (bytes_remaining + max_msg_size) / max_msg_size;

    while (bytes_remaining)
    {
        msghdr msg;
        iovec io[2];

        uint32_t frag_hdr[4] = {_frame_id, frag_id++, num_fragments, (uint32_t)current_pos};

        io[0].iov_base = frag_hdr;
        io[0].iov_len = sizeof frag_hdr;

        size_t payload_size = std::min(max_msg_size, bytes_remaining);

        io[1].iov_base = frame->buffer.data() + current_pos;
        io[1].iov_len = payload_size;

        memcpy(msg.msg_name, &_client_sa, sizeof _client_sa);
        msg.msg_namelen = sizeof _client_sa;
        msg.msg_iov = io;
        msg.msg_iovlen = 2;
        msg.msg_control = 0;
        msg.msg_controllen = 0;
        msg.msg_flags = 0;

        if (sendmsg(_dgram_fd, &msg, 0) == -1)
        {
            err(1, "sendmsg");
        }

        current_pos += payload_size;
        bytes_remaining -= payload_size;
    }

    ++_frame_id;
}

