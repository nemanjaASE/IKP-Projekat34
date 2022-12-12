#include "Threads.h"

DWORD WINAPI client_th(LPVOID param) {
	
	ClientInformation* client_information = (ClientInformation*)param;

	SOCKET client_socket = client_information->client_socket;
	DWORD thread_id = *(client_information->lp_thread_id);
	HANDLE thread_handle = *(client_information->handle_client_thread);
	RingBuffer* ring_buffer = client_information->ring_buffer;
	HashTable* students = client_information->students;

	int bytes_recieved = 0;
	int header_size = 3 * sizeof(uint8_t);
	int body_size = 0;
	int i_result = 0;
	Header header;

	while (1) {

		bytes_recieved = 0;
		i_result = 0;
		body_size = 0;

		do
		{
			select_function(client_socket, READ);
			i_result = recv(client_socket, (char*)&header + bytes_recieved, header_size - bytes_recieved, 0);
			if (i_result == SOCKET_ERROR || i_result == 0)
			{
				break;
			}
			bytes_recieved += i_result;

		} while (bytes_recieved < header_size);


		body_size = header.first_name_len + header.last_name_len + header.index_len;
		printf("[Thread_ID:%lu] The client sent payload message length: %d\n", thread_id, body_size);

		Student* student = create_student();
		bytes_recieved = 0;
		char* buffer = (char*)malloc(sizeof(char) * body_size + 1);

		if (IS_NULL(buffer)) {
			return -1;
		}

		do
		{
			select_function(client_socket, READ);
			i_result = recv(client_socket, buffer + bytes_recieved, body_size - bytes_recieved, 0);
			if (i_result == SOCKET_ERROR || i_result == 0)
			{
				break;
			}
			bytes_recieved += i_result;

		} while (bytes_recieved < body_size);
		buffer[body_size] = '\0';

		deserialize_student(student, buffer, header);

		if (!ht_add(students, student->index, *student)) {
			printf("[Thread_ID:%lu] Student with '%s' already exists!\n", thread_id, student->index);
		}
		else {
			printf("[Thread_ID:%lu] The student '%s:%s:%s' successfully inserted.\n", thread_id, student->first_name
				, student->last_name
				, student->index);
		}
		printf("[Thread_ID:%lu] ", thread_id);
		ht_show(students);
		printf("\n");

	}
	return 0;
}

ClientInformation* init_client_information(DWORD* thread_id, HANDLE* thread_handle, HashTable* students, RingBuffer* ring_buffer) {

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
	client_information->handle_client_thread = thread_handle;

	return client_information;
}

void set_client_socket(ClientInformation* client_information, SOCKET socket) {
	client_information->client_socket = socket;
}