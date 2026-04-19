//
// fuzz_frame.cpp -- libFuzzer target for the length-prefixed frame parser.
//
// Feeds arbitrary bytes through read_frame() to detect buffer overflows,
// integer overflows in the length prefix, or other memory safety issues.
//
// Build:
//   cmake -B build -S callJvmThreadpool -DENABLE_FUZZ=ON -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
//   cmake --build build --target fuzz_frame
//
// Run:
//   ./build/fuzz_frame -max_total_time=60
//

#include <cstdint>
#include <cstring>
#include <unistd.h>

#include "frame.h"

// Create a pipe, write fuzz data to the write end, read a frame from the read
// end. This exercises the full read_frame() code path including the 4-byte
// length prefix parsing and payload read.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size == 0) return 0;

    int pipefd[2];
    if (pipe(pipefd) != 0) return 0;

    int read_fd = pipefd[0];
    int write_fd = pipefd[1];

    // Write fuzz data to the pipe (non-blocking to avoid deadlock on large
    // inputs -- we cap at a reasonable size so write() won't block).
    constexpr size_t MAX_WRITE = 64 * 1024;
    size_t to_write = size < MAX_WRITE ? size : MAX_WRITE;
    ssize_t written = write(write_fd, data, to_write);
    close(write_fd);  // Close write end so read_frame sees EOF after data

    if (written <= 0) {
        close(read_fd);
        return 0;
    }

    // Try to read a frame -- this is the function under test.
    // Use a reasonably sized buffer to catch overflows.
    constexpr size_t BUF_SIZE = 8192;
    uint8_t buf[BUF_SIZE];
    memset(buf, 0, BUF_SIZE);

    int result = read_frame(read_fd, buf, BUF_SIZE);
    (void)result;  // We don't care about the return value, just that it doesn't crash

    close(read_fd);
    return 0;
}
