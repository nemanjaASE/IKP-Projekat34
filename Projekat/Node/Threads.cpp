#include "Threads.h"

#pragma region Threads

DWORD WINAPI exit_th(LPVOID param) {

	HANDLE* exit_signal = (HANDLE*)(param);

	char server_end;

	printf("\n[Exit_Thread] Press ESC to close server..\n");

	do {

		server_end = _getch();
	} while (server_end != 27);

	printf("[Exit_Thread] Server is closing down...\n");



	ReleaseSemaphore(*exit_signal, 1, NULL);


	return 0;
}

DWORD WINAPI client_th(LPVOID param) {
	ClientInformation* client_information = (ClientInformation*)param;

	uint8_t bytes_recieved = 0;
	int header_size = 3 * sizeof(uint8_t);
	uint8_t body_size = 0;
	int i_result = 0;
	Header header;
	int end = 0;
	Student* student = create_student();

	while (1) {

		bytes_recieved = 0;
		i_result = 0;
		body_size = 0;

		do
		{
			select_function(client_information->client_socket, READ, client_information->exit_semaphore);
			i_result = recv(client_information->client_socket, (char*)&header + bytes_recieved, header_size - bytes_recieved, 0);
			if (i_result == SOCKET_ERROR || i_result == 0){
				end = 1;
				break;
			}
			bytes_recieved += i_result;

		} while (bytes_recieved < header_size);

		if (end == 1) {
			break;
		}

		body_size = header.first_name_len + header.last_name_len + header.index_len;
		printf("[Thread_ID:%lu] The client sent payload message length: %d\n", *client_information->lp_thread_id, body_size);

		bytes_recieved = 0;
		char* buffer = (char*)malloc(sizeof(char) * body_size + 1);

		if (IS_NULL(buffer)) {
			free(student);
			return -1;
		}

		do
		{
			select_function(client_information->client_socket, READ, client_information->exit_semaphore);
			i_result = recv(client_information->client_socket, buffer + bytes_recieved, body_size - bytes_recieved, 0);
			if (i_result == SOCKET_ERROR || i_result == 0){
				end = 1;
				break;
			}
			bytes_recieved += i_result;

		} while (bytes_recieved < body_size);
		buffer[body_size] = '\0';

		if (end == 1) {
			free(buffer);
			free_student(student);
			break;
		}

		deserialize_student(student, buffer, header);

		if (client_information->nodes->counter == 0) {

			// Empty nodes list

			if (!ht_add(client_information->students, student->index, *student)) {
				printf("[Thread_ID:%lu] Student with '%s' already exists!\n", *client_information->lp_thread_id, student->index);
			}
			else {
				printf("[Thread_ID:%lu] The student '%s:%s:%s' successfully inserted.\n", *client_information->lp_thread_id, student->first_name
					, student->last_name
					, student->index);
			}
		}
		else {

		}



		printf("[Thread_ID:%lu] ", *client_information->lp_thread_id);
		ht_show(client_information->students);
		free(buffer);
	}

	free_student(student);
	ReleaseSemaphore(client_information->has_client_semaphore, 1, NULL);
	printf("[Thread_ID:%lu] Terminating...\n", *client_information->lp_thread_id);

	return 0;
}

DWORD WINAPI integrity_update_th(LPVOID param) {

	NodeInformation* node_information = (NodeInformation*)param;

	char number = -1;
	int i_result = 0;

	select_function(node_information->node_socket, READ);
	i_result = recv(node_information->node_socket, (char*)&number, sizeof(char), 0);

	if ((number - '0') == SEND) {

		printf("[Thread_ID:%lu] Intregrity update starting...\n", node_information->lp_thread_id);

		unsigned int bytes_sent = 0;
		char* message_to_send = (char*)malloc(sizeof(unsigned long));
		sprintf(message_to_send, "%lu", node_information->students->counter);

		// Send number of students

		do{
			select_function(node_information->node_socket, WRITE);

			i_result = send(node_information->node_socket, message_to_send + bytes_sent, sizeof(unsigned long) - bytes_sent, 0);
			if (i_result == SOCKET_ERROR || i_result == 0)
			{
				i_result = shutdown(node_information->node_socket, SD_SEND);
				if (i_result == SOCKET_ERROR)
				{
					printf("Shutdown failed with error: %d\n", WSAGetLastError());
				}

				closesocket(node_information->node_socket);
				free_node_information(node_information);
				free(message_to_send);
				printf("[Thread_ID:%lu] Terminating...\n", node_information->lp_thread_id);
				return -1;
			}
			bytes_sent += i_result;

		} while (bytes_sent < sizeof(unsigned long));

		free(message_to_send);

		unsigned char* header = (unsigned char*)malloc(sizeof(Header));
		if (IS_NULL(header)) {
			free_node_information(node_information);
			printf("[Thread_ID:%lu] Terminating...\n", node_information->lp_thread_id);
			return -1;
		}

		size_t header_size = 3 * sizeof(uint8_t);
		char* body = NULL;
		int end = 0;

		for (size_t i = 0; i < node_information->students->size; i++) {

			if (node_information->students->entries[i] == NULL)
				continue;

			HashTableEntry* current = node_information->students->entries[i];

			while (current != NULL) {

				size_t body_len = fill_header(*current->student, header);
				unsigned int bytes_sent = 0;

				// send header
				do {
					select_function(node_information->node_socket, WRITE);

					i_result = send(node_information->node_socket, (char*)header + bytes_sent, header_size - bytes_sent, 0);

					if (i_result == SOCKET_ERROR || i_result == 0)
					{
						i_result = shutdown(node_information->node_socket, SD_SEND);
						if (i_result == SOCKET_ERROR)
						{
							printf("Shutdown failed with error: %d\n", WSAGetLastError());
						}
						end = 1;
						break;
					}

					bytes_sent += i_result;

				} while (bytes_sent < header_size);

				if (end == 1) {
					break;
				}

				body = serialize_student(current->student);
				bytes_sent = 0;

				// Body send

				do {
					select_function(node_information->node_socket, WRITE);

					i_result = send(node_information->node_socket, body + bytes_sent, body_len - bytes_sent, 0);

					if (i_result == SOCKET_ERROR || i_result == 0)
					{
						i_result = shutdown(node_information->node_socket, SD_SEND);
						if (i_result == SOCKET_ERROR)
						{
							printf("Shutdown failed with error: %d\n", WSAGetLastError());
						}
						end = 1;
						break;
					}

					bytes_sent += i_result;

				} while (bytes_sent < body_len);

				free(body);

				if (end == 1) {
					break;
				}

				current = current->next_entry;
				Sleep(1000);
			}
		}
		free(header);

		printf("[Thread_ID:%lu] Intregrity update successfully ended!\n", node_information->lp_thread_id);
	}
	
	if (sll_insert_first(node_information->nodes, node_information->node_socket)) {
		printf("[Thread_ID:%lu] New node successfully added.\n", node_information->lp_thread_id);
	}
	else {
		printf("[Thread_ID:%lu] Adding new node failed.\n", node_information->lp_thread_id);
	}

	DWORD old_id = node_information->lp_thread_id;
	DWORD new_id;
	HANDLE new_handle = CreateThread(NULL, 0, &node_th, node_information, 0, &new_id);
	if (IS_NULL(new_handle)) {
		free_node_information(node_information);
		return -1;
	}
	else {
		node_information->node_thread_handle = new_handle;
		node_information->lp_thread_id = new_id;
		printf("[Thread_ID:%lu] A new Node_thread with ID=%lu has been started.\n", old_id, node_information->lp_thread_id);
	}

	printf("[Thread_ID:%lu] Terminating...\n", old_id);

	return 0;
}

DWORD WINAPI node_th(LPVOID param) {

	NodeInformation* node_information = (NodeInformation*)param;
	printf("[Thread_ID:%lu] Waiting...\n", node_information->lp_thread_id);

	int i_result = -1;
	int end = 0;
	char number = -1;

	while (1) {

		select_function(node_information->node_socket, READ, node_information->exit_semaphore);

		i_result = recv(node_information->node_socket, (char*)&number, sizeof(char), 0);
		if (i_result == SOCKET_ERROR || i_result == 0) {
			end = 1;
			break;
		}

		if ((number - '0') == NODE_DISC) {
			printf("[Thread_ID:%lu] Node %d disconeccted...\n", node_information->lp_thread_id, node_information->node_socket);
			if (sll_delete(node_information->nodes, node_information->node_socket)) {
				printf("[Thread_ID:%lu] Node %d deleted...Nodes: %d\n", node_information->lp_thread_id, node_information->node_socket, node_information->nodes->counter);
				closesocket(node_information->node_socket);
			}

			break;
		}
	}

	printf("[Thread_ID:%lu] Terminating...\n", node_information->lp_thread_id);
	free_node_information(node_information);
	return 0;
}

#pragma endregion Threads

#pragma region ClientInformation

ClientInformation* init_client_information(LPDWORD thread_id, HashTable* students, RingBuffer* ring_buffer, SinglyLinkedList* nodes, HANDLE has_client_semaphore, HANDLE exit_semaphore) {

	if (IS_NULL(students) || IS_NULL(ring_buffer) || IS_NULL(nodes)) {
		return NULL;
	}

	ClientInformation* client_information = (ClientInformation*)malloc(sizeof(ClientInformation));
	if (IS_NULL(client_information)) {
		return NULL;
	}

	client_information->students = students;
	client_information->ring_buffer = ring_buffer;
	client_information->nodes = nodes;
	client_information->lp_thread_id = thread_id;
	client_information->has_client_semaphore = has_client_semaphore;
	client_information->exit_semaphore = exit_semaphore;

	return client_information;
}

void set_client_socket(ClientInformation* client_information, SOCKET socket) {

	if (IS_NULL(client_information)) {
		return;
	}

	client_information->client_socket = socket;
}

void free_client_information(ClientInformation* client_information) {

	SAFE_HANDLE(client_information->exit_semaphore);
	SAFE_HANDLE(client_information->has_client_semaphore);
	closesocket(client_information->client_socket);

	free(client_information);
}

#pragma endregion ClientInformation

#pragma region NodeInformation

NodeInformation* init_node_information(HashTable* students, SinglyLinkedList* nodes, RingBuffer* ring_buffer, HANDLE exit_semaphore) {

	if (IS_NULL(students) || IS_NULL(nodes) || IS_NULL(ring_buffer)) {
		return NULL;
	}

	NodeInformation* node_information = (NodeInformation*)malloc(sizeof(NodeInformation));
	if (IS_NULL(node_information)) {
		return NULL;
	}

	node_information->nodes = nodes;
	node_information->students = students;
	node_information->ring_buffer = ring_buffer;
	node_information->exit_semaphore = exit_semaphore;

	return node_information;
}

void set_node_socket(NodeInformation* node_information, SOCKET socket, WORD thread_id, HANDLE node_thread_handle) {

	if (IS_NULL(node_information)) {
		return;
	}

	node_information->node_socket = socket;
	node_information->lp_thread_id = thread_id;
	node_information->node_thread_handle = node_thread_handle;
}

void free_node_information(NodeInformation* node_information) {
	
	SAFE_HANDLE(node_information->node_thread_handle);
	free(node_information);
}

#pragma endregion NodeInformation