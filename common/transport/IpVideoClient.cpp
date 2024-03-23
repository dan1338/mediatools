#include "transport/IpVideoClient.h"
#include "VideoFrame.h"
#include <cstring>
#include <cstdio>
#include <err.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "log.h"

static int connect_(int fd, const struct sockaddr *addr, socklen_t len)
{
    return connect(fd, addr, len);
}

IpVideoClient::IpVideoClient(const std::string &connect_addr, int connect_port):
    _frame_format{nullptr}, _last_frame_id{0}, _expected_num_fragments{0}
{
    _connect_sa.sin_family = AF_INET;
    _connect_sa.sin_addr.s_addr = inet_addr(connect_addr.c_str());
    _connect_sa.sin_port = htons(connect_port);

    _stream_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (_stream_fd == -1)
        ERROR("socket(STREAM) -> -1 (%s)\n", strerror(errno));

    _dgram_fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (_dgram_fd == -1)
        ERROR("socket(DGRAM) -> -1 (%s)\n", strerror(errno));
}

auto IpVideoClient::connect() -> void
{
    if (connect_(_stream_fd, (sockaddr*)&_connect_sa, sizeof _connect_sa) == -1)
        ERROR("connect() -> 1 (%s)\n", strerror(errno));

    uint16_t recv_port = htons(ntohs(_connect_sa.sin_port) + 1);

    sockaddr_in dgram_sa;
    dgram_sa.sin_family = AF_INET;
    dgram_sa.sin_addr.s_addr = inet_addr("0.0.0.0");
    dgram_sa.sin_port = recv_port;

    if (bind(_dgram_fd, (sockaddr*)&dgram_sa, sizeof dgram_sa) == -1)
        ERROR("bind() -> -1\n");

    if (send(_stream_fd, &recv_port, sizeof recv_port, 0) != sizeof recv_port)
        ERROR("send() -> -1\n");

    VideoFrame::Format frame_format;

    if (recv(_stream_fd, &frame_format, sizeof frame_format, MSG_WAITALL) != sizeof frame_format)
        ERROR("recv() -> -1\n");

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
    size_t total_size = 0;
    uint8_t got_num_fragments = 0;
    uint32_t frame_id;

    msghdr msg = {};
    iovec io[2];

    uint32_t frag_hdr[4];

    io[0].iov_base = frag_hdr;
    io[0].iov_len = sizeof frag_hdr;

    std::array<uint8_t, 65535-2000> tmp;

    io[1].iov_base = tmp.data();
    io[1].iov_len = tmp.size();

	msg.msg_iov = io;
	msg.msg_iovlen = 2;

    do
    {
        ssize_t recv_size;

		do
		{
			if (recv_size = recvmsg(_dgram_fd, &msg, 0); recv_size == -1)
			{
				err(1, "recvmsg");
			}

			DEBUG("[.] recvmsg -> %zd\n", recv_size);
		}
		while (!recv_size);

        frame_id = frag_hdr[0];
        uint32_t frag_id = frag_hdr[1];
        uint32_t expect_num_frag = frag_hdr[2];
        uint32_t frag_offset = frag_hdr[3];

        DEBUG("- frame_id = %d\n", frame_id);
        DEBUG("- frag_id = %d\n", frag_id);
        DEBUG("- expect_num_frag = %d\n", expect_num_frag);
        DEBUG("- frag_offset = %d\n", frag_offset);

        if (frame_id != _last_frame_id || _expected_num_fragments == 0)
        {
			DEBUG("[.] set frame_id = %d\n", frame_id);

            _last_frame_id = frame_id;
            _expected_num_fragments = expect_num_frag;
            got_num_fragments = 1;
        }
        else
        {
			DEBUG("[.] got fragment = %d, (%d offset)\n", frag_id, frag_offset);

            ++got_num_fragments;
        }

        size_t payload_size = (recv_size - sizeof frag_hdr);
        total_size += payload_size;

        memcpy(_frame_buffer.data() + frag_offset, tmp.data(), payload_size);
    }
    while (got_num_fragments < _expected_num_fragments);

    auto video_frame = std::make_shared<VideoFrame>();
    video_frame->buffer.resize(total_size);
    video_frame->format = *_frame_format;

    memcpy(video_frame->buffer.data(), _frame_buffer.data(), total_size);

    INFO("received frame (%d id, %d fragments, %zu size)\n", frame_id, got_num_fragments, total_size);

    return video_frame;
}

auto IpVideoClient::get_frame_format() -> VideoFrame::Format
{
    if (!_frame_format)
        ERROR("client not connected");

    return *_frame_format.get();
}

