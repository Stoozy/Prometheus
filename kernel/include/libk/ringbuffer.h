#pragma once

#include <stdint.h>
#include <unistd.h>

typedef struct ring_buffer {
    int head;
    int tail;

    void* buffer;

    size_t cap;     // max amount of data
    size_t tsize;   // size of actual data type
    size_t len;
} RingBuffer;



void rb_init(RingBuffer * rb, size_t cap, size_t tsize);
void rb_destroy(RingBuffer * rb);
int rb_push(RingBuffer * rb, void * item);
int rb_pop(RingBuffer * rb, void * item);


