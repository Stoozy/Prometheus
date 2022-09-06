#pragma once

#include <stdint.h>
#include <unistd.h>

typedef struct ring_buffer {
    int write_pos;
    int read_pos;
    void* buffer;
    size_t len;
} RingBuffer;


