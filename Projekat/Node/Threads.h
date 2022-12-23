 #pragma once

#ifndef THREADS_H
#define THREADS_H

#define _CRT_SECURE_NO_WARNINGS 1

#include "NodeHelper.h"
#include "HashTable.h"
#include "RingBuffer.h"
#include "SinglyLinkedList.h"

#define SAFE_HANDLE(a) if(a){CloseHandle(a);}

typedef enum message_type_t {
	TERMINATE = 0,
	SEND
} MessageType;

typedef struct client_information_t {

	SOCKET client_socket;
	LPDWORD lp_thread_id;
	HashTable* students;
	RingBuffer* ring_buffer;
	HANDLE has_client_semaphore;
	HANDLE exit_semaphore;

} ClientInformation;

typedef struct node_information_t {

	SOCKET node_socket;
	WORD lp_thread_id;
	HANDLE node_thread_handle;
	HashTable* students;
	SinglyLinkedList* nodes;

} NodeInformation;

ClientInformation* init_client_information(LPDWORD thread_id, HashTable* students, RingBuffer* ring_buffer, HANDLE has_client_semaphore, HANDLE exit_semaphore);

void set_client_socket(ClientInformation* client_information, SOCKET socket);

void free_client_information(ClientInformation* client_information);

NodeInformation* init_node_information(HashTable* students, SinglyLinkedList* nodes);

void set_node_socket(NodeInformation* node_information, SOCKET socket, WORD thread_id, HANDLE node_thread_handle);

void free_node_information(NodeInformation* node_information);

DWORD WINAPI exit_th(LPVOID param);

DWORD WINAPI client_th(LPVOID param);

DWORD WINAPI integrity_update_th(LPVOID param);

#endif // !THREADS_H
