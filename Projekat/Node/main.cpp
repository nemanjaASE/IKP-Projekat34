#include "Threads.h"
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

int main() {

	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	if (!initialize_windows_sockets()) {
		return -1;
	}

	SOCKET client_listen_socket = INVALID_SOCKET;
	SOCKET node_listen_socket = INVALID_SOCKET;
	sockaddr_in client_address;
	sockaddr_in node_address;

	// Data Structures

	SinglyLinkedList* nodes = sll_create();
	HashTable* students = ht_create(100);
	RingBuffer* ring_buffer = rb_create(50);
	if (IS_NULL(students) || IS_NULL(nodes) || IS_NULL(ring_buffer)) {

		sll_free(nodes);
		ht_free(students);
		rb_free(ring_buffer);
		closesocket(client_listen_socket);
		closesocket(node_listen_socket);
		WSACleanup();

		return 0;
	}

	unsigned long client_port = 0;
	unsigned long node_port = 0;
	
	client_port = nh_port_number_input((char*)"CLIENT");
	node_port = nh_port_number_input((char*)"NODE");

	getchar();

	// Firt node on network or n-th

	HANDLE has_client_semaphore = CreateSemaphore(0, 1, 1, NULL);
	HANDLE exit_signal = CreateSemaphore(0, 0, 64, NULL);
	if (IS_NULL(has_client_semaphore) || IS_NULL(exit_signal)) {
		sll_free(nodes);
		ht_free(students);
		rb_free(ring_buffer);
		closesocket(client_listen_socket);
		closesocket(node_listen_socket);
		WSACleanup();

		return 0;
	}


	if (!nh_integrity_update(nodes, students)) {
		rb_free(ring_buffer);
		sll_free(nodes);
		ht_free(students);
		return -1;
	}
	else {
		
		system("cls");

		if (!nh_init_listen_socket(&client_listen_socket, &client_address, client_port)) {
			sll_free(nodes);
			rb_free(ring_buffer);
			ht_free(students);
			return -1;
		}
		else {
			printf("------------------------------------------\n");
			printf("[CLIENT LISTEN SOCKET] '%s':'%lu' |\n",
				inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
			printf("------------------------------------------\n");
		}

		if (!nh_init_listen_socket(&node_listen_socket, &node_address, node_port)) {
			sll_free(nodes);
			ht_free(students);
			rb_free(ring_buffer);
			return -1;
		}
		else {
			printf("[NODE LISTEN SOCKET] '%s':'%lu'   |\n",
				inet_ntoa(node_address.sin_addr), ntohs(node_address.sin_port));
			printf("------------------------------------------\n");
		}

		Node* current = nodes->head;
		while (current != NULL) {

			DWORD new_id;

			NodeInformation* node_information = init_node_information(students, nodes, ring_buffer, exit_signal);
			if (IS_NULL(node_information)) {
				break;
			}

			HANDLE new_handle = CreateThread(NULL, 0, &node_th, node_information, 0, &new_id);
			if (IS_NULL(new_handle)) {
				free_node_information(node_information);
				return -1;
			}

			printf("[Receive_thread] A new Node_thread with ID=%lu has been started.\n", new_id);

			set_node_socket(node_information, current->node_socket, new_id, new_handle);
			Sleep(100);

			current = current->next_node;
		}
	}
	
	ht_show(students);
	// Exit_Thread

	DWORD thread_exit_id = -1;
	HANDLE thread_exit_handle;
	HANDLE exit_signal_semaphore = CreateSemaphore(0, 0, 1, NULL);

	thread_exit_handle = CreateThread(NULL, 0, &exit_th, &exit_signal_semaphore, 0, &thread_exit_id);

	if (IS_NULL(thread_exit_handle)) {

		sll_free(nodes);
		rb_free(ring_buffer);
		ht_free(students);
		closesocket(client_listen_socket);
		closesocket(node_listen_socket);
		WSACleanup();

		return 0;
	}

	// Client information

	DWORD thread_id = -1;
	HANDLE thread_handle = NULL;

	ClientInformation* client_information = init_client_information(&thread_id, students, ring_buffer, nodes, has_client_semaphore, exit_signal);
	if (IS_NULL(client_information)) {
		sll_free(nodes);
		ht_free(students);
		rb_free(ring_buffer);
		closesocket(client_listen_socket);
		closesocket(node_listen_socket);
		WSACleanup();

		return -1;
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
				SOCKET accepted_socket = accept_new_socket(node_listen_socket);

				NodeInformation* node_information = init_node_information(students, nodes, ring_buffer, exit_signal);
				if (IS_NULL(node_information)) {
					break;
				}

				DWORD new_id;
				HANDLE new_handle = CreateThread(NULL, 0, &integrity_update_th, node_information, 0, &new_id);
				if (IS_NULL(new_handle)) {
					break;
				}

				printf("[Receive_thread] A new Integrity_update_thread with ID=%lu has been started.\n", new_id);
				set_node_socket(node_information, accepted_socket, new_id, new_handle);
			}
		}
	}

	Node* current = nodes->head;
	char message = '1';

	while (current != NULL) {

		select_function(current->node_socket, WRITE);

		int i_result = send(current->node_socket, (char*)&message, sizeof(char), 0);

		if (i_result == SOCKET_ERROR || i_result == 0)
		{
			i_result = shutdown(current->node_socket, SD_SEND);
			if (i_result == SOCKET_ERROR)
			{
				printf("Shutdown failed with error: %d\n", WSAGetLastError());
			}
			closesocket(current->node_socket);
			break;
		}
		current = current->next_node;
	}

	// Terminate all threads
	if (!IS_NULL(thread_handle)) {
		ReleaseSemaphore(exit_signal, 1 + nodes->counter, NULL);

		WaitForSingleObject(thread_handle, INFINITE);

		SAFE_HANDLE(thread_handle);
	}
	else {
		ReleaseSemaphore(exit_signal, nodes->counter, NULL);
		Sleep(2000);
	}

	SAFE_HANDLE(thread_exit_handle);
	SAFE_HANDLE(exit_signal_semaphore);

	ht_free(students);
	rb_free(ring_buffer);
	sll_free(nodes);
	free_client_information(client_information);

	closesocket(client_listen_socket);
	closesocket(node_listen_socket);
	WSACleanup();

	printf("\n\n Press any key to exit.");
	
	char c = getchar();
 	c = getchar();

	return 0;
}