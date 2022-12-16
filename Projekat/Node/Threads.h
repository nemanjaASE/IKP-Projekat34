 #pragma once

#ifndef THREADS_H
#define THREADS_H

#include "NodeHelper.h"
#include "HashTable.h"
#include "RingBuffer.h"

#define SAFE_HANDLE(a) if(a){CloseHandle(a);}

typedef struct client_information_t {

	SOCKET client_socket;
	LPDWORD lp_thread_id;
	HashTable* students;
	RingBuffer* ring_buffer;
	HANDLE has_client_semaphore;
	HANDLE exit_semaphore;

} ClientInformation;

ClientInformation* init_client_information(LPDWORD thread_id, HashTable* students, RingBuffer* ring_buffer, HANDLE has_client_semaphore, HANDLE exit_semaphore);

void set_client_socket(ClientInformation* client_information, SOCKET socket);

void free_client_information(ClientInformation* client_information);

DWORD WINAPI exit_th(LPVOID param);

DWORD WINAPI client_th(LPVOID param);

#endif // !THREADS_H
