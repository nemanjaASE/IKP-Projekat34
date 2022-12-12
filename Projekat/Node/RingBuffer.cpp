#include "RingBuffer.h"

RingBuffer* rb_create(size_t size) {

	if (size <= 0) {
		return NULL;
	}

	RingBuffer* ring_buffer = NULL;

	ring_buffer = (RingBuffer*)malloc(sizeof(RingBuffer));
	if (IS_NULL(ring_buffer)) {
		printf("Not enough memory!");
		return ring_buffer;
	}

	ring_buffer->buffer = (DistributedTransaction*)malloc(size * sizeof(DistributedTransaction));
	if (IS_NULL(ring_buffer->buffer)) {
		printf("Not enough memory!");
		free(ring_buffer);
		return NULL;
	}

	ring_buffer->pop_idx = 0;
	ring_buffer->push_idx = 0;
	ring_buffer->size = size;

	return ring_buffer;
}

void rb_push_value(RingBuffer* ring_buffer, DistributedTransaction value) {
	ring_buffer->buffer[ring_buffer->push_idx].id = value.id;
	ring_buffer->buffer[ring_buffer->push_idx].student = value.student;
	ring_buffer->push_idx = (ring_buffer->push_idx + 1) % ring_buffer->size;
}

DistributedTransaction rb_pop_value(RingBuffer* ring_buffer) {
	DistributedTransaction element = ring_buffer->buffer[ring_buffer->pop_idx];
	ring_buffer->pop_idx = (ring_buffer->pop_idx + 1) % ring_buffer->size;
	return element;
}

void rb_free(RingBuffer* ring_buffer) {

	if (IS_NULL(ring_buffer->buffer)) {
		return;
	}
	free(ring_buffer->buffer);

	if (IS_NULL(ring_buffer)) {
		return;
	}
	free(ring_buffer);
}