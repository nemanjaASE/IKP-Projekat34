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

	char* buffer = (char*)malloc(sizeof(char) * (FIRST_NAME_MAX + LAST_NAME_MAX + INDEX_MAX + 1));
	if (IS_NULL(buffer)) {
		return -1;
	}

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

			ht_show(client_information->students);
		}
		else {

			DistributedTransaction distributed_transaction;
			distributed_transaction.student = *student;

			rb_push_value(client_information->ring_buffer, distributed_transaction);

			ReleaseSemaphore(client_information->ring_buffer_semaphore, 1, NULL);
		}

		Sleep(100);
	}

	free(buffer);
	free_student(student);
	ReleaseSemaphore(client_information->has_client_semaphore, 1, NULL);
	printf("[Thread_ID:%lu] Terminating...\n", *client_information->lp_thread_id);

	return 0;
}

DWORD WINAPI integrity_update_th(LPVOID param) {

	NodeInformation* node_information = (NodeInformation*)param;
	DWORD thread_id = node_information->lp_thread_id;
	VoteList* votes = node_information->votes;

	char number = -1;
	int i_result = 0;

	char* message_to_send = (char*)malloc(sizeof(unsigned long));
	if (IS_NULL(message_to_send)) {
		free(node_information);
		return -1;
	}

	unsigned char* header = (unsigned char*)malloc(sizeof(Header));
	if (IS_NULL(header)) {

		free(message_to_send);
		free(node_information);
		return -1;
	}

	select_function(node_information->node_socket, READ);
	i_result = recv(node_information->node_socket, (char*)&number, sizeof(char), 0);

	if ((number - '0') == SEND) {

		printf("[Thread_ID:%lu] Intregrity update starting...\n", thread_id);

		sprintf(message_to_send, "%lu", node_information->students->counter);

		// Send number of students

		nh_send_number_of_students(node_information->node_socket, message_to_send);

		size_t header_size = 3 * sizeof(uint8_t);
		char* body = NULL;

		for (size_t i = 0; i < node_information->students->size; i++) {

			if (node_information->students->entries[i] == NULL)
				continue;

			HashTableEntry* current = node_information->students->entries[i];

			while (current != NULL) {

				size_t body_len = fill_header(*current->student, header);
				unsigned int bytes_sent = 0;

				// Send header
				nh_send_header(node_information->node_socket, header);

				body = serialize_student(current->student);

				// Send body

				nh_send_student(node_information->node_socket, body, body_len);

				free(body);

				current = current->next_entry;
				Sleep(50);
			}
		}

		printf("[Thread_ID:%lu] Intregrity update successfully ended!\n", thread_id);
	}
	
	if (sll_insert_first(node_information->nodes, node_information->node_socket)) {

		printf("[Thread_ID:%lu] New node successfully added.\n", thread_id);

		NodeInformation* new_node_information = init_node_information(node_information->students
			, node_information->nodes
			, node_information->ring_buffer
			, node_information->exit_semaphore
			, node_information->handles
			, node_information->votes);

		if (IS_NULL(new_node_information)) {
			free_node_information(node_information);
			return 0;
		}

		set_node_socket(new_node_information, node_information->node_socket);

		DWORD old_id = thread_id;
		free(node_information);
		DWORD new_id;
		HANDLE new_handle = CreateThread(NULL, 0, &node_th, new_node_information, 0, &new_id);
		if (IS_NULL(new_handle)) {
			return -1;
		}
		else {

			set_node_thread(new_node_information, new_id, new_handle);

			hl_insert_first(new_node_information->handles, new_handle);

			printf("[Thread_ID:%lu] A new Node_thread with ID=%lu has been started.\n", old_id, new_id);
		}

		printf("[Thread_ID:%lu] Terminating...\n", old_id);
	}
	else {
		printf("[Thread_ID:%lu] Adding new node failed.\n", thread_id);
	}

	free(message_to_send);
	free(header);

	return 0;
}

DWORD WINAPI node_th(LPVOID param) {

	Sleep(1000);

	NodeInformation* node_information = (NodeInformation*)param;
	SOCKET connected_node = node_information->node_socket;
	DWORD thread_id = node_information->lp_thread_id;
	SinglyLinkedList* nodes = node_information->nodes;
	HashTable* students = node_information->students;
	HandleList* handles = node_information->handles;
	VoteList* votes = node_information->votes;

	Student* student = create_student();
	if (IS_NULL(student)) {
		return -1;
	}

	char* buffer = (char*)malloc(FIRST_NAME_MAX + LAST_NAME_MAX + INDEX_MAX + 1);
	if (IS_NULL(buffer)) {
		free_student(student);
		return -1;
	}

	int i_result = -1;
	char number = -1;

	while (1) {

		select_function(connected_node, READ, node_information->exit_semaphore);

		i_result = recv(connected_node, (char*)&number, sizeof(char), 0);
		if (i_result == SOCKET_ERROR || i_result == 0) {
			break;
		}


		if ((number - '0') == NODE_DISC) {

			printf("[Thread_ID:%lu] Node %llu disconeccted...\n", thread_id
				, connected_node);

			if (sll_delete(nodes, connected_node)) {

				hl_delete(handles, node_information->node_thread_handle);
				printf("[Thread_ID:%lu] Node %llu deleted...Nodes: %u\n", thread_id
					, connected_node
					, node_information->nodes->counter);
			}
			hl_delete(handles, node_information->node_thread_handle);
			closesocket(connected_node);

			break;
		}
		else if ((number - '0') == START) {

			printf("[Thread_ID:%lu] Start Transaction...\n", thread_id);

			int body_size = 0;
			Header header;

			//Receive header

			nh_receive_header(connected_node, &header);
			body_size = header.first_name_len + header.last_name_len + header.index_len;
			printf("The node sent payload message length: %d\n", body_size);

			// Receive body

			nh_receive_student(connected_node, buffer, body_size);

			// Get student from raw values

			deserialize_student(student, buffer, header);

			// Check

			char answer = '6'; // No

			if (!ht_find_by_key(students, student->index)) {
				answer = '5';
			}

			Sleep(1000);

			select_function(connected_node, WRITE);

			int i_result = send(connected_node, (char*)&answer, sizeof(char), 0);

			if (i_result == SOCKET_ERROR || i_result == 0)
			{
				printf("Send Error!\n");
				i_result = shutdown(connected_node, SD_SEND);
				if (i_result == SOCKET_ERROR)
				{
					printf("Shutdown failed with error: %d\n", WSAGetLastError());
				}
				closesocket(connected_node);
				break;
			};
		}
		else if((number - '0') == YES)
		{
			vl_insert_first(votes, 1);
			Sleep(100);
			ReleaseSemaphore(votes->vote_signal, 1, NULL);
			
		}
		else if ((number - '0') == NO)
		{
			vl_insert_first(votes, 0);
			Sleep(100);
			ReleaseSemaphore(votes->vote_signal, 1, NULL);
		}
		else if ((number - '0') == ROLLBACK) {

			printf("[Thread_ID:%lu] Rollback transaction...\n", thread_id);
		}
		else if ((number - '0') == COMMIT) {

			printf("[Thread_ID:%lu] Commit transaction\n", thread_id);

			if (ht_add(students, student->index, *student)) {

				printf("The student '%s:%s:%s' successfully inserted.\n", student->first_name
					, student->last_name
					, student->index);

				ht_show(students);
			}
		}
	}

	printf("[Thread_ID:%lu] Terminating...\n", thread_id);

	free_student(student);
	free(buffer);
	free_node_information(node_information);

	return 0;
}

DWORD WINAPI coordinator_th(LPVOID param) {

	CoordinatorInformation* coordinator_information = (CoordinatorInformation*)param;
	DWORD thread_id = *coordinator_information->lp_thread_id;
	RingBuffer* ring_buffer = coordinator_information->ring_buffer;

	DistributedTransaction distributed_transaction;
	Student student;

	HANDLE semaphores[2] = 
			{coordinator_information->exit_semaphore, coordinator_information->ring_buffer_semaphore};

	unsigned char* header = (unsigned char*)malloc(sizeof(Header));
	if (IS_NULL(header)) {
		printf("[Thread_ID:%lu] Terminating...\n", thread_id);
		return -1;
	}

	while (WaitForMultipleObjects(2, semaphores, false, INFINITE) == (WAIT_OBJECT_0 + 1)) {

		distributed_transaction = rb_pop_value(ring_buffer);
		student = distributed_transaction.student;

		printf("[Thread_ID:%lu] DISTRIBUTED TRANSACTION STARTED\n", thread_id);
		Node* head = coordinator_information->nodes->head;

		// Send start message

		nh_send_start_message(head);
		
		// Send header

		size_t body_len = fill_header(student, header);

		nh_send_header(head, header);

		// Send student

		char* body = serialize_student(&student);

		nh_send_student(head, body, (int)body_len);

		free(body);

		int i = 0;
		int number_of_nodes = coordinator_information->nodes->counter;

		while (WaitForSingleObject(coordinator_information->votes->vote_signal, INFINITE) == WAIT_OBJECT_0) {

			i++;

			if (i == number_of_nodes) {
				break;
			}
		}

		if (!vl_check_votes(coordinator_information->votes)) {
			// Rollback
			Sleep(200);
			nh_send_decision(head, (char*)'3');
		}
		else {
			// Commit
			Sleep(200);
			nh_send_decision(head, (char*)'4');

			//Apply changes locally
			ht_add(coordinator_information->students, student.index, student);
		}

		Sleep(500);

		vl_clear(coordinator_information->votes);

		printf("[Thread_ID:%lu] END OF DISTRIBUTED TRANSACTION\n", thread_id);
		ht_show(coordinator_information->students);
	}

	free(header);
	printf("[Thread_ID:%lu] Ended\n", thread_id);

	return 0;
}

#pragma endregion Threads

#pragma region ClientInformation

ClientInformation* init_client_information(LPDWORD thread_id, HashTable* students, RingBuffer* ring_buffer, SinglyLinkedList* nodes, HANDLE has_client_semaphore, HANDLE exit_semaphore, HANDLE ring_buffer_semaphore) {

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
	client_information->ring_buffer_semaphore = ring_buffer_semaphore;

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

NodeInformation* init_node_information(HashTable* students, SinglyLinkedList* nodes, RingBuffer* ring_buffer, HANDLE exit_semaphore, HandleList* handles, VoteList* votes) {

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
	node_information->votes = votes;
	node_information->exit_semaphore = exit_semaphore;
	node_information->handles = handles;

	return node_information;
}

void set_node_socket(NodeInformation* node_information, SOCKET socket) {

	if (IS_NULL(node_information)) {
		return;
	}

	node_information->node_socket = socket;
}

void set_node_thread(NodeInformation* node_information, DWORD thread_id, HANDLE node_thread_handle) {
	node_information->lp_thread_id = thread_id;
	node_information->node_thread_handle = node_thread_handle;
}

void free_node_information(NodeInformation* node_information) {
	free(node_information);
}

#pragma endregion NodeInformation

#pragma region CoordinatorInformation

CoordinatorInformation* init_coordinator_information(LPDWORD thread_id, HashTable* students, RingBuffer* ring_buffer, SinglyLinkedList* nodes, HANDLE exit_semaphore, HANDLE ring_buffer_semaphore, VoteList* votes) {

	CoordinatorInformation* coordinator_information = (CoordinatorInformation*)malloc(sizeof(CoordinatorInformation));
	if (IS_NULL(coordinator_information)) {
		return NULL;
	}

	coordinator_information->students = students;
	coordinator_information->ring_buffer = ring_buffer;
	coordinator_information->nodes = nodes;
	coordinator_information->lp_thread_id = thread_id;
	coordinator_information->exit_semaphore = exit_semaphore;
	coordinator_information->ring_buffer_semaphore = ring_buffer_semaphore;
	coordinator_information->votes = votes;

	return coordinator_information;
}

#pragma endregion CoordinatorInformation