#pragma once

#include "VideoFrame.h"
#include "transport/IVideoTx.h"
#include <arpa/inet.h>
#include <memory>
#include <string>
#include <sys/socket.h>

class IpVideoServer : public IVideoTx
{
public:
    IpVideoServer(const std::string &listen_addr, int listen_port);

    auto set_frame_format(VideoFrame::Format format) -> void override;

    auto handle_control_message(const ControlMessageHandler &handler) -> void override;
    auto await_connection() -> void override;
    auto poll_client() -> void override;

    auto send_frame(const VideoFramePtr &frame) -> void override;

private:
    std::unique_ptr<VideoFrame::Format> _frame_format;
    std::unique_ptr<ControlMessageHandler> _control_message_handler;

    sockaddr_in _listen_sa;
    sockaddr_in _client_sa;

    int _listen_fd;
    int _stream_fd;
    int _dgram_fd;

    uint32_t _frame_id;
};

