#pragma once

#include <cstddef>
#include <cstdint>

// Length-prefixed framing protocol.
// Each frame is: [4-byte big-endian length] [payload of that length]
// Maximum frame size: 16 MiB.

constexpr size_t FRAME_MAX_LEN = 16 * 1024 * 1024;

// Write a complete frame to fd.
// Returns 0 on success, -1 on error.
int write_frame(int fd, const void* data, uint32_t len);

// Read a complete frame from fd.
// Writes up to maxlen bytes into buf. Returns actual payload length,
// or -1 on error (including frame too large for buf).
int read_frame(int fd, void* buf, size_t maxlen);
