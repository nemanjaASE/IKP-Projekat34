#pragma once

#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdio.h>
#include <stdlib.h>
#include <ws2tcpip.h>
#include "DistributedTransaction.h"

#define IS_NULL(a) (a == NULL)

typedef struct ring_buffer_t {
	size_t size;
	unsigned int push_idx;
	unsigned int pop_idx;
	DistributedTransaction* buffer;
	CRITICAL_SECTION rb_cs;
} RingBuffer;

RingBuffer* rb_create(size_t size);

void rb_push_value(RingBuffer* ring_buffer, DistributedTransaction value);

DistributedTransaction rb_pop_value(RingBuffer* ring_buffer);

void rb_free(RingBuffer* ring_buffer);

#endif // !RING_BUFFER_H
