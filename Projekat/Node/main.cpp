#include "Threads.h"
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

int main() {

	_CrtMemState sOld;
	_CrtMemState sNew;
	_CrtMemState sDiff;
	_CrtMemCheckpoint(&sOld); //take a snapshot

	if (!initialize_windows_sockets()) {
		return -1;
	}

	SOCKET client_listen_socket = INVALID_SOCKET;
	SOCKET node_listen_socket = INVALID_SOCKET;
	sockaddr_in client_address;
	sockaddr_in node_address;

	unsigned long client_port = 0;
	unsigned long node_port = 0;
	
	client_port = nh_port_number_input((char*)"CLIENT");
	node_port = nh_port_number_input((char*)"NODE");

	system("cls");

	if (!nh_init_listen_socket(&client_listen_socket, &client_address, client_port)) {
		return -1;
	}
	else {
		printf("------------------------------------------\n");
		printf("[CLIENT LISTEN SOCKET] '%s':'%lu' |\n",
			inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
		printf("------------------------------------------\n");
	}

	if (!nh_init_listen_socket(&node_listen_socket, &node_address, node_port)) {
		return -1;
	}
	else {
		printf("[NODE LISTEN SOCKET] '%s':'%lu'   |\n",
			inet_ntoa(node_address.sin_addr), ntohs(node_address.sin_port));
		printf("------------------------------------------\n");
	}
	
	// Exit_Thread

	DWORD thread_exit_id = -1;
	HANDLE thread_exit_handle;
	HANDLE exit_signal_semaphore = CreateSemaphore(0, 0, 1, NULL);

	thread_exit_handle = CreateThread(NULL, 0, &exit_th, &exit_signal_semaphore, 0, &thread_exit_id);

	if (IS_NULL(thread_exit_handle)) {

		closesocket(client_listen_socket);
		closesocket(node_listen_socket);
		WSACleanup();

		return 0;
	}

	// Data Structures

	HashTable* students = ht_create(100);
	RingBuffer* ring_buffer = rb_create(50);
	if (IS_NULL(students) || IS_NULL(ring_buffer)) {

		ht_free(students);
		rb_free(ring_buffer);
		closesocket(client_listen_socket);
		closesocket(node_listen_socket);
		WSACleanup();

		return 0;
	}

	// Client information

	DWORD thread_id = -1;
	HANDLE thread_handle = NULL;
	HANDLE has_client_semaphore = CreateSemaphore(0, 1, 1, NULL);
	HANDLE exit_signal = CreateSemaphore(0, 0, 1, NULL);
	if (IS_NULL(has_client_semaphore) || IS_NULL(exit_signal)) {
		ht_free(students);
		rb_free(ring_buffer);
		closesocket(client_listen_socket);
		closesocket(node_listen_socket);
		WSACleanup();

		return 0;
	}

	ClientInformation* client_information = init_client_information(&thread_id, students, ring_buffer, has_client_semaphore, exit_signal);
	if (IS_NULL(client_information)) {
		ht_free(students);
		rb_free(ring_buffer);
		closesocket(client_listen_socket);
		closesocket(node_listen_socket);
		WSACleanup();

		return 0;
	}

	// Driver code

	fd_set read_fds;
	FD_ZERO(&read_fds);

	timeval timeVal;
	timeVal.tv_sec = 2;
	timeVal.tv_usec = 0;

	int client_counter = 0;
	int i_result = 0;

	while (WaitForSingleObject(exit_signal_semaphore, 10) == WAIT_TIMEOUT) {

		FD_ZERO(&read_fds);

		if (WaitForSingleObject(has_client_semaphore, 10) == WAIT_OBJECT_0) {

			client_counter = 0;
			if (!IS_NULL(thread_handle)) {
				SAFE_HANDLE(thread_handle);
				closesocket(client_information->client_socket);
				thread_handle = NULL;
			}
			printf("[Receive_thread] Waiting for new client.\n");
		}

		if (client_counter == 0) {

			FD_SET(client_listen_socket, &read_fds);
		}

		FD_SET(node_listen_socket, &read_fds);

		i_result = select(0, &read_fds, NULL, NULL, &timeVal);

		if (i_result == SOCKET_ERROR) {
			printf("Error %d \n", WSAGetLastError());
			break;
		}
		else if (i_result == 0) {
			continue;
		}
		else {

			if (FD_ISSET(client_listen_socket, &read_fds)) {
				// Client connected
				SOCKET accepted_socket = accept_new_socket(client_listen_socket);
				set_client_socket(client_information, accepted_socket);

				thread_handle = CreateThread(NULL, 0, &client_th, client_information, 0, &thread_id);
				printf("[Receive_thread] A new Client_thread with ID=%lu has been started.\n", thread_id);

				client_counter = 1;
			}

			if (FD_ISSET(node_listen_socket, &read_fds)) {
				// Node connected
			}
		}
	}

	// Terminate all threads
	if (!IS_NULL(thread_handle)) {
		ReleaseSemaphore(exit_signal, 1, NULL);

		WaitForSingleObject(thread_handle, INFINITE);

		SAFE_HANDLE(thread_handle);
	}
	
	SAFE_HANDLE(thread_exit_handle);
	SAFE_HANDLE(exit_signal_semaphore);

	ht_free(students);
	rb_free(ring_buffer);
	free_client_information(client_information);

	closesocket(client_listen_socket);
	closesocket(node_listen_socket);
	WSACleanup();

	printf("\n\n Press any key to exit.");

	_CrtMemCheckpoint(&sNew); //take a snapshot 
	if (_CrtMemDifference(&sDiff, &sOld, &sNew)) // if there is a difference
	{
		OutputDebugString(L"-----------_CrtMemDumpStatistics ---------");
		_CrtMemDumpStatistics(&sDiff);
		OutputDebugString(L"-----------_CrtMemDumpAllObjectsSince ---------");
		_CrtMemDumpAllObjectsSince(&sOld);
		OutputDebugString(L"-----------_CrtDumpMemoryLeaks ---------");
		_CrtDumpMemoryLeaks();
	}

	char c = getchar();
 	c = getchar();

	return 0;
}