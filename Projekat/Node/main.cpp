#include "Threads.h"
#include <stdlib.h>
#include <crtdbg.h>

int main() {

	_getch();

	if (!initialize_windows_sockets()) {
		return -1;
	}

	SOCKET client_listen_socket = INVALID_SOCKET;
	SOCKET node_listen_socket = INVALID_SOCKET;
	sockaddr_in client_address;
	sockaddr_in node_address;

	// Data Structures
	HandleList* handles = hl_create();
	if (IS_NULL(handles)) {
		closesocket(client_listen_socket);
		closesocket(node_listen_socket);
		WSACleanup();
	}

	SinglyLinkedList* nodes = sll_create();
	HashTable* students = ht_create(50);
	RingBuffer* ring_buffer = rb_create(50);
	VoteList* votes = vl_create();
	if (IS_NULL(students) || IS_NULL(nodes) || IS_NULL(ring_buffer) || IS_NULL(votes)) {

		vl_free(votes);
		sll_free(nodes);
		ht_free(students);
		rb_free(ring_buffer);
		hl_free(handles);
		closesocket(client_listen_socket);
		closesocket(node_listen_socket);
		WSACleanup();

		return 0;
	}


	unsigned long client_port = 0;
	unsigned long node_port = 0;
	
	client_port = nh_port_number_input((char*)"CLIENT");
	node_port = nh_port_number_input((char*)"NODE");

	char c = getchar();

	// First node on network or n-th

	HANDLE has_client_semaphore = CreateSemaphore(0, 1, 1, NULL);
	HANDLE exit_signal = CreateSemaphore(0, 0, 64, NULL);
	HANDLE ring_buffer_semaphore = CreateSemaphore(0, 0, 20, NULL);
	if (IS_NULL(has_client_semaphore) || IS_NULL(exit_signal) || IS_NULL(ring_buffer_semaphore)) {

		vl_free(votes);
		sll_free(nodes);
		ht_free(students);
		rb_free(ring_buffer);
		hl_free(handles);
		closesocket(client_listen_socket);
		closesocket(node_listen_socket);
		WSACleanup();

		return 0;
	}

	if (!nh_integrity_update(nodes, students)) {
		SAFE_HANDLE(has_client_semaphore);
		SAFE_HANDLE(exit_signal);
		vl_free(votes);
		rb_free(ring_buffer);
		sll_free(nodes);
		hl_free(handles);
		ht_free(students);
		closesocket(client_listen_socket);
		closesocket(node_listen_socket);
		WSACleanup();

		return -1;
	}
	else {
		
		system("cls");

		if (!nh_init_listen_socket(&client_listen_socket, &client_address, client_port)) {
			sll_free(nodes);
			ht_free(students);
			rb_free(ring_buffer);
			hl_free(handles);
			closesocket(client_listen_socket);
			closesocket(node_listen_socket);
			WSACleanup();
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
			hl_free(handles);
			closesocket(client_listen_socket);
			closesocket(node_listen_socket);
			WSACleanup();
			return -1;
		}
		else {
			printf("[NODE LISTEN SOCKET] '%s':'%lu'   |\n",
				inet_ntoa(node_address.sin_addr), ntohs(node_address.sin_port));
			printf("------------------------------------------\n");
		}

		Node* current = nodes->head;
		DWORD new_id;
		while (current != NULL) {

			NodeInformation* node_information = init_node_information(students
																	 , nodes
																	 , ring_buffer
																	 , exit_signal
																	 , handles
																	 , votes);
			if (IS_NULL(node_information)) {
				break;
			}

			set_node_socket(node_information, current->node_socket);

			HANDLE new_handle = CreateThread(NULL, 0, &node_th, node_information, 0, &new_id);
			if (IS_NULL(new_handle)) {
				sll_free(nodes);
				ht_free(students);
				rb_free(ring_buffer);
				hl_free(handles);
				closesocket(client_listen_socket);
				closesocket(node_listen_socket);
				free_node_information(node_information);
				WSACleanup();
				return -1;
			}
			printf("\t[Receive] A new Node_thread with ID=%lu has been started.\n", new_id);

			set_node_thread(node_information, new_handle);
			hl_insert_first(handles, new_handle);
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

		SAFE_HANDLE(has_client_semaphore);
		SAFE_HANDLE(exit_signal);
		vl_free(votes);
		hl_free(handles);
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

	ClientInformation* client_information = init_client_information(students, ring_buffer, nodes, has_client_semaphore, exit_signal, ring_buffer_semaphore);
	if (IS_NULL(client_information)) {

		SAFE_HANDLE(thread_exit_handle);
		SAFE_HANDLE(has_client_semaphore);
		SAFE_HANDLE(exit_signal);
		vl_free(votes);
		hl_free(handles);
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

	// Coordinator thread
	DWORD coordinator_id = -1;
	CoordinatorInformation* coordinator_information = init_coordinator_information(  students
																				   , ring_buffer
																				   , nodes
																				   , exit_signal
																				   , ring_buffer_semaphore
																				   , votes);

	HANDLE coordinator_handle = CreateThread(NULL, 0, &coordinator_th, coordinator_information, 0, &coordinator_id);
	if (IS_NULL(coordinator_handle)) {

		SAFE_HANDLE(thread_exit_handle);
		SAFE_HANDLE(has_client_semaphore);
		SAFE_HANDLE(exit_signal);
		free_client_information(client_information);
		free(coordinator_information);
		vl_free(votes);
		hl_free(handles);
		sll_free(nodes);
		ht_free(students);
		rb_free(ring_buffer);
		closesocket(client_listen_socket);
		closesocket(node_listen_socket);
		WSACleanup();

		return -1;
	}

	hl_insert_first(handles, coordinator_handle);

	while (WaitForSingleObject(exit_signal_semaphore, 10) == WAIT_TIMEOUT) {

		FD_ZERO(&read_fds);

		if (WaitForSingleObject(has_client_semaphore, 10) == WAIT_OBJECT_0) {
			// Client disconnected

			client_counter = 0;

			if (!IS_NULL(thread_handle)) {
				SAFE_HANDLE(thread_handle);
				closesocket(client_information->client_socket);
				thread_handle = NULL;
			}
			printf("\t[Receive] Waiting for new client.\n");
		}

		if (client_counter == 0) {

			FD_SET(client_listen_socket, &read_fds);
		}

		FD_SET(node_listen_socket, &read_fds);

		i_result = select(0, &read_fds, NULL, NULL, &timeVal);

		if (i_result == SOCKET_ERROR) {
			printf("\t[Receive] Error %d \n", WSAGetLastError());
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
				printf("\t[Receive] A new Client_thread with ID=%lu has been started.\n", thread_id);

				client_counter = 1;
			}

			if (FD_ISSET(node_listen_socket, &read_fds)) {
				// Node connected

				SOCKET accepted_socket = accept_new_socket(node_listen_socket);

				NodeInformation* node_information = init_node_information(  students
																		  , nodes
																		  , ring_buffer
																		  , exit_signal
																		  , handles
																		  , votes);
				if (IS_NULL(node_information)) {
					break;
				}

				set_node_socket(node_information, accepted_socket);

				DWORD new_id;
				HANDLE new_handle = CreateThread(NULL, 0, &integrity_update_th, node_information, 0, &new_id);
				if (IS_NULL(new_handle)) {
					break;
				}

				printf("\t[Receive] A new Integrity_update_thread with ID=%lu has been started.\n", new_id);
				set_node_thread(node_information, new_handle);
			}
		}
	}
	
	if (!IS_NULL(thread_handle)) {

		// Client still connected

		ReleaseSemaphore(exit_signal, 2 + nodes->counter, NULL);

		WaitForSingleObject(thread_handle, INFINITE);

		SAFE_HANDLE(thread_handle);
	}
	else {
		// Client disconnected

		ReleaseSemaphore(exit_signal, 1 + nodes->counter, NULL);
	}

	// Wait for threads safe exit

	wait_for_all_threads(handles);

	Sleep(1000);

	// Notify other nodes

	graceful_exit(nodes->head);

	printf("\n\n Press any key to exit.");

	 c = getchar();
	 c = getchar();

	SAFE_HANDLE(thread_exit_handle);
	SAFE_HANDLE(exit_signal_semaphore);
	SAFE_HANDLE(exit_signal);
	SAFE_HANDLE(has_client_semaphore);
	SAFE_HANDLE(ring_buffer_semaphore);

	free(coordinator_information);
	ht_free(students);
	rb_free(ring_buffer);
	sll_free(nodes);
	hl_free(handles);
	vl_free(votes);
	free_client_information(client_information);

	closesocket(client_listen_socket);
	closesocket(node_listen_socket);
	WSACleanup();

	getchar();

	return 0;
}