 #pragma once

#ifndef THREADS_H
#define THREADS_H

#include "NodeHelper.h"
#include "HashTable.h"
#include "RingBuffer.h"

typedef struct client_information_t {

	SOCKET client_socket;
	DWORD* lp_thread_id;
	HANDLE* handle_client_thread;
	HashTable* students;
	RingBuffer* ring_buffer;

} ClientInformation;

ClientInformation* init_client_information(DWORD* thread_id, HANDLE* thread_handle, HashTable* students, RingBuffer* ring_buffer);
void set_client_socket(ClientInformation* client_information, SOCKET socket);

DWORD WINAPI client_th(LPVOID param);

#endif // !THREADS_H
