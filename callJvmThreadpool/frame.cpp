#include "frame.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstdio>
#include <unistd.h>

// Helper: write exactly n bytes to fd.
static int write_all(int fd, const void* buf, size_t n) {
    const auto* p = static_cast<const uint8_t*>(buf);
    size_t remaining = n;
    while (remaining > 0) {
        ssize_t written = write(fd, p, remaining);
        if (written < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        p += written;
        remaining -= static_cast<size_t>(written);
    }
    return 0;
}

// Helper: read exactly n bytes from fd.
static int read_all(int fd, void* buf, size_t n) {
    auto* p = static_cast<uint8_t*>(buf);
    size_t remaining = n;
    while (remaining > 0) {
        ssize_t got = read(fd, p, remaining);
        if (got < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (got == 0) return -1;  // EOF
        p += got;
        remaining -= static_cast<size_t>(got);
    }
    return 0;
}

int write_frame(int fd, const void* data, uint32_t len) {
    if (len > FRAME_MAX_LEN) {
        fprintf(stderr, "write_frame: payload too large (%u > %zu)\n", len, FRAME_MAX_LEN);
        return -1;
    }
    uint32_t net_len = htonl(len);
    if (write_all(fd, &net_len, sizeof(net_len)) < 0) return -1;
    if (len > 0 && write_all(fd, data, len) < 0) return -1;
    return 0;
}

int read_frame(int fd, void* buf, size_t maxlen) {
    uint32_t net_len;
    if (read_all(fd, &net_len, sizeof(net_len)) < 0) return -1;
    uint32_t len = ntohl(net_len);
    if (len > FRAME_MAX_LEN) {
        fprintf(stderr, "read_frame: frame too large (%u > %zu)\n", len, FRAME_MAX_LEN);
        return -1;
    }
    if (len > maxlen) {
        fprintf(stderr, "read_frame: frame (%u) exceeds buffer (%zu)\n", len, maxlen);
        return -1;
    }
    if (len > 0 && read_all(fd, buf, len) < 0) return -1;
    return static_cast<int>(len);
}
