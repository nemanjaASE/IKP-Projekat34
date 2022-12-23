#include "Threads.h"

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

		if (!ht_add(client_information->students, student->index, *student)) {
			printf("[Thread_ID:%lu] Student with '%s' already exists!\n", *client_information->lp_thread_id, student->index);
		}
		else {
			printf("[Thread_ID:%lu] The student '%s:%s:%s' successfully inserted.\n", *client_information->lp_thread_id, student->first_name
				, student->last_name
				, student->index);
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
	printf("[Thread_ID:%lu] The node sent: %c\n", node_information->lp_thread_id, number);

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
			}
		}
		free(header);

		printf("[Thread_ID:%lu] Intregrity update successfully ended!\n", node_information->lp_thread_id);
	}

	printf("[Thread_ID:%lu] Terminating...\n", node_information->lp_thread_id);

	free_node_information(node_information);

	return 0;
}

ClientInformation* init_client_information(LPDWORD thread_id, HashTable* students, RingBuffer* ring_buffer, HANDLE has_client_semaphore, HANDLE exit_semaphore) {

	if (IS_NULL(students) || IS_NULL(ring_buffer)) {
		return NULL;
	}

	ClientInformation* client_information = (ClientInformation*)malloc(sizeof(ClientInformation));
	if (IS_NULL(client_information)) {
		return NULL;
	}

	client_information->students = students;
	client_information->ring_buffer = ring_buffer;
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

NodeInformation* init_node_information(HashTable* students, SinglyLinkedList* nodes) {

	if (IS_NULL(students) || IS_NULL(nodes)) {
		return NULL;
	}

	NodeInformation* node_information = (NodeInformation*)malloc(sizeof(NodeInformation));
	if (IS_NULL(node_information)) {
		return NULL;
	}

	node_information->nodes = nodes;
	node_information->students = students;

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