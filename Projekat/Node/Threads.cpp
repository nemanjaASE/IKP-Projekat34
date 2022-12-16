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
	if (!IS_NULL(student)) {
		free(student);
	}

	ReleaseSemaphore(client_information->has_client_semaphore, 1, NULL);
	printf("[Thread_ID:%lu] Terminating...\n", *client_information->lp_thread_id);

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
	client_information->client_socket = socket;
}

void free_client_information(ClientInformation* client_information) {

	SAFE_HANDLE(client_information->exit_semaphore);
	SAFE_HANDLE(client_information->has_client_semaphore);
	closesocket(client_information->client_socket);

	free(client_information);
}