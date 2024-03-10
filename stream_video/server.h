#include "video_source/IVideoSource.h"
#include <arpa/inet.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include <err.h>

class VideoServer
{
private:
    VideoFormat _format;
    int _listen_fd;
    int _con_fd;
    int _dgram_fd;

    sockaddr_in _listen_sa;
    sockaddr_in _client_sa;

public:
    VideoServer(const std::string bind_addr, int bind_port, VideoFormat format)
    {
        _format.width = htons(format.width);
        _format.height = htons(format.height);
        _format.num_channels = format.num_channels;
        _format.bits_per_pixels = format.bits_per_pixels;

        _listen_fd = socket(AF_INET, SOCK_STREAM, 0);
        _dgram_fd = socket(AF_INET, SOCK_DGRAM, 0);

        int reuse_addr = 1;
        setsockopt(_listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof reuse_addr);

        _listen_sa.sin_family = AF_INET;
        _listen_sa.sin_addr.s_addr = inet_addr(bind_addr.c_str());
        _listen_sa.sin_port = htons(bind_port);
    }

    void await_connection()
    {
        if (bind(_listen_fd, (sockaddr*)&_listen_sa, sizeof _listen_sa) == -1)
            err(1, "bind");

        listen(_listen_fd, 1);

        socklen_t socklen = sizeof(sockaddr_in);
        _con_fd = accept(_listen_fd, (sockaddr*)&_client_sa, &socklen);

        send(_con_fd, &_format, sizeof _format, MSG_WAITALL);
    }

    void send_frame(uint8_t *buf, size_t size)
    {
        ssize_t ret;

        ret = sendto(_dgram_fd, buf, size, 0, (sockaddr*)&_client_sa, sizeof _client_sa);

        if (ret != size)
            warn("sendto -> %ld (want %ld)", ret, size);
    }
};
