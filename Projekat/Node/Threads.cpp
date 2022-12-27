#include "Threads.h"

#pragma region Threads

DWORD WINAPI exit_th(LPVOID param) {

	HANDLE* exit_signal = (HANDLE*)(param);

	char server_end;

	printf("\n\t[Exit] Press ESC to close server..\n");

	do {

		server_end = _getch();

	} while (server_end != 27);

	printf("\t[Exit] Server is closing down...\n");

	ReleaseSemaphore(*exit_signal, 1, NULL);

	return 0;
}

DWORD WINAPI client_th(LPVOID param) {

	ClientInformation* client_information = (ClientInformation*)param;

	unsigned int body_size = 0;
	int i_result = 0;
	Header header;
	int end = 0;
	DistributedTransaction distributed_transaction;
	DWORD thread_id = GetCurrentThreadId();

	char* buffer = (char*)malloc(sizeof(char) * (FIRST_NAME_MAX + LAST_NAME_MAX + INDEX_MAX + 1));
	if (IS_NULL(buffer)) {
		return -1;
	}

	Student* student = create_student();

	while (1) {

		if (!nh_receive_header(client_information->client_socket, &header, client_information->exit_semaphore)) {
			break;
		}

		body_size = header.first_name_len + header.last_name_len + header.index_len;
		printf("\t[Client:%lu] The client sent payload message length: %d\n", thread_id
																		    , body_size);

		if (!nh_receive_student(client_information->client_socket, buffer, body_size, client_information->exit_semaphore)) {
			break;
		}

		deserialize_student(student, buffer, header);

		if (client_information->nodes->counter == 0) {

			// Empty nodes list

			if (!ht_add(client_information->students, student->index, *student)) {
				printf("\t[Client:%lu] Student with '%s' already exists!\n", thread_id
																		   , student->index);
			}
			else {
				printf("\t[Client:%lu] The student '%s:%s:%s' successfully inserted.\n", thread_id
																					   , student->first_name
																					   , student->last_name
																					   , student->index);
			}

			ht_show(client_information->students);
		}
		else {

			distributed_transaction.student = *student;

			rb_push_value(client_information->ring_buffer, distributed_transaction);

			ReleaseSemaphore(client_information->ring_buffer_semaphore, 1, NULL);
		}

		Sleep(100);
	}

	free(buffer);
	free_student(student);
	ReleaseSemaphore(client_information->has_client_semaphore, 1, NULL);
	printf("\t[Client:%lu] Terminating...\n", thread_id);

	return 0;
}

DWORD WINAPI integrity_update_th(LPVOID param) {

	NodeInformation* node_information = (NodeInformation*)param;
	VoteList* votes = node_information->votes;

	int i_result = -1;
	size_t header_size = 3 * sizeof(uint8_t);
	unsigned int body_len = 0;
	char number;
	char* body = NULL;
	HashTableEntry* current;
	unsigned int bytes_sent;
	DWORD new_thread_id;
	DWORD thread_id = GetCurrentThreadId();
	HANDLE new_thread_handle = NULL;

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

		printf("\t[IntegrityUpdate:%lu] Intregrity update starting...\n", thread_id);

		sprintf(message_to_send, "%lu", node_information->students->counter);

		// Send number of students

		nh_send_number_of_students(node_information->node_socket, message_to_send);

		body = NULL;

		for (size_t i = 0; i < node_information->students->size; i++) {

			if (node_information->students->entries[i] == NULL)
				continue;

			current = node_information->students->entries[i];

			while (current != NULL) {

				body_len = (unsigned int)fill_header(*current->student, header);
				bytes_sent = 0;

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

		printf("\t[IntegrityUpdate:%lu] Intregrity update successfully ended!\n", thread_id);
	}
	
	if (sll_insert_first(node_information->nodes, node_information->node_socket)) {

		printf("\t[IntegrityUpdate:%lu] New node successfully added.\n", thread_id);

		NodeInformation* new_node_information = init_node_information(  node_information->students
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

		free(node_information);

		new_thread_handle = CreateThread(NULL, 0, &node_th, new_node_information, 0, &new_thread_id);
		if (IS_NULL(new_thread_handle)) {
			return -1;
		}
		else {

			set_node_thread(new_node_information, new_thread_handle);

			hl_insert_first(new_node_information->handles, new_thread_handle);

			printf("\t[IntegrityUpdate:%lu] A new Node_thread with ID=%lu has been started.\n", thread_id
																							  , new_thread_id);
		}

		printf("\t[IntegrityUpdate:%lu] Terminating...\n", thread_id);
	}
	else {
		printf("\t[IntegrityUpdate:%lu] Adding new node failed.\n", thread_id);
	}

	free(message_to_send);
	free(header);

	return 0;
}

DWORD WINAPI node_th(LPVOID param) {

	NodeInformation* node_information = (NodeInformation*)param;

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
	unsigned int body_size = 0;
	char number = -1;
	char option;
	char answer;
	BOOL end = false;
	Header header;
	DWORD thread_id = GetCurrentThreadId();

	SOCKET connected_node = node_information->node_socket;
	SinglyLinkedList* nodes = node_information->nodes;
	HashTable* students = node_information->students;
	HandleList* handles = node_information->handles;
	VoteList* votes = node_information->votes;

	while (!end) {

		if (select_function(connected_node, READ, node_information->exit_semaphore) != 1){
			break;
		}

		i_result = recv(connected_node, (char*)&number, sizeof(char), 0);
		if (i_result == SOCKET_ERROR || i_result == 0) {
			break;
		}

		option = (number - '0');

		switch(option)
		{	
			case NODE_DISC:
			{
				printf("\t[Node:%lu] Node %llu disconeccted...\n", thread_id
															     , connected_node);

				if (sll_delete(nodes, connected_node)) {

					hl_delete(handles, node_information->node_thread_handle);
					printf("\t[Node:%lu] Node %llu deleted...Nodes: %u\n", thread_id
																		 , connected_node
																		 , node_information->nodes->counter);

					hl_delete(handles, node_information->node_thread_handle);
					closesocket(connected_node);
				}

				end = true;

				break;
			}
			case START:
			{
				printf("\t[Node:%lu] Start Transaction...\n", thread_id);

				body_size = 0;

				//Receive header

				nh_receive_header(connected_node, &header);
				body_size = (unsigned int)(header.first_name_len + header.last_name_len + header.index_len);
				printf("\t[Node:%lu] The node sent payload message length: %d\n", thread_id
																				, body_size);

				// Receive body

				nh_receive_student(connected_node, buffer, body_size);

				// Get student from raw values

				deserialize_student(student, buffer, header);

				// Check

				answer = '6'; // No

				if (!ht_find_by_key(students, student->index)) {
					answer = '5';
				}

				Sleep(200);

				select_function(connected_node, WRITE);

				i_result = -1;

				i_result = send(connected_node, (char*)&answer, sizeof(char), 0);

				if (i_result == SOCKET_ERROR || i_result == 0)
				{
					printf("\t[Node:%lu] Send Error!\n", GetCurrentThreadId());
					i_result = shutdown(connected_node, SD_SEND);
					if (i_result == SOCKET_ERROR)
					{
						printf("\t[Node:%lu] Shutdown failed with error: %d\n", GetCurrentThreadId()
							, WSAGetLastError());
					}
					closesocket(connected_node);
				}

				break;
			}
			case YES:
			{
				vl_insert_first(votes, 1);
				Sleep(10);
				ReleaseSemaphore(votes->vote_signal, 1, NULL);

				break;
			}
			case NO:
			{
				vl_insert_first(votes, 0);
				Sleep(10);
				ReleaseSemaphore(votes->vote_signal, 1, NULL);

				break;
			}
			case ROLLBACK:
			{
				printf("\t[Node:%lu] Rollback transaction...\n", thread_id);

				break;
			}
			case COMMIT:
			{
				printf("\t[Node:%lu] Commit transaction\n", thread_id);

				if (ht_add(students, student->index, *student)) {

					printf("\t[Node:%lu] The student '%s:%s:%s' successfully inserted.\n", thread_id
																						 , student->first_name
																					     , student->last_name
																						 , student->index);

					ht_show(students);
				}

				break;
			}
			default:
			{
				printf("\t[Node:%lu] Unknown command!\n", thread_id);

				break;
			}
		};

	}

	printf("\t[Node:%lu] Terminating...\n", thread_id);

	free_student(student);
	free(buffer);
	free_node_information(node_information);

	return 0;
}

DWORD WINAPI coordinator_th(LPVOID param) {

	CoordinatorInformation* coordinator_information = (CoordinatorInformation*)param;
	RingBuffer* ring_buffer = coordinator_information->ring_buffer;

	DistributedTransaction distributed_transaction;
	Student student;
	int i = 0;
	int number_of_nodes;
	char* body = NULL;
	BOOL end = false;
	DWORD wait_result;
	DWORD thread_id = GetCurrentThreadId();
	Node* head = NULL;
	unsigned int body_len = 0;
	HANDLE vote_signal = coordinator_information->votes->vote_signal;

	HANDLE semaphores[2] = 
			{coordinator_information->exit_semaphore, coordinator_information->ring_buffer_semaphore};

	unsigned char* header = (unsigned char*)malloc(sizeof(Header));
	if (IS_NULL(header)) {
		printf("\t[Coordinator:%lu] Terminating...\n", thread_id);
		return -1;
	}

	while (WaitForMultipleObjects(2, semaphores, false, INFINITE) == (WAIT_OBJECT_0 + 1)) {

		distributed_transaction = rb_pop_value(ring_buffer);
		student = distributed_transaction.student;

		printf("\t[Coordinator:%lu] DISTRIBUTED TRANSACTION STARTED\n", thread_id);
		head = coordinator_information->nodes->head;

		// Send start message

		if (!nh_send_start_message(head)) {
			continue;
		}
		
		// Send header

		body_len = (unsigned int)fill_header(student, header);

		if (body_len <= 0) {
			continue;
		}

		if (!nh_send_header(head, header)) {
			continue;
		}

		// Send student

		body = serialize_student(&student);

		if (IS_NULL(body)) {
			continue;
		}

		if (!nh_send_student(head, body, (int)body_len)) {
			free(body);
			continue;
		}

		free(body);

		i = 0;
		number_of_nodes = coordinator_information->nodes->counter;
		end = false;

		while (!end) {

			wait_result = WaitForSingleObject(vote_signal, 5000);

			switch (wait_result)
			{
				case WAIT_OBJECT_0:
				{
					i++;

					if (i == number_of_nodes) {
						end = true;
					}

					break;
				}
				case WAIT_TIMEOUT: {

					printf("\t[Coordinator:%lu] Timeout \n", thread_id);

					end = true;

					break;
				}
				default: {

					printf("\t[Coordinator:%lu] Error %lu \n", thread_id
															 , GetLastError());

					break;
				}
			}
		}

		if (!vl_check_votes(coordinator_information->votes)) {
			// Rollback
			Sleep(20);
			nh_send_decision(head, (char*)'3');
		}
		else {
			// Commit
			Sleep(20);
			nh_send_decision(head, (char*)'4');

			//Apply changes locally
			ht_add(coordinator_information->students, student.index, student);
		}

		Sleep(30);

		vl_clear(coordinator_information->votes);

		printf("\t[Coordinator:%lu] END OF DISTRIBUTED TRANSACTION\n", thread_id);
		ht_show(coordinator_information->students);
	}

	free(header);
	printf("\t[Coordinator:%lu] Ended\n", thread_id);

	return 0;
}

#pragma endregion Threads

#pragma region ClientInformation

ClientInformation* init_client_information(HashTable* students, RingBuffer* ring_buffer, SinglyLinkedList* nodes, HANDLE has_client_semaphore, HANDLE exit_semaphore, HANDLE ring_buffer_semaphore) {

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

void set_node_thread(NodeInformation* node_information, HANDLE node_thread_handle) {
	node_information->node_thread_handle = node_thread_handle;
}

void free_node_information(NodeInformation* node_information) {
	free(node_information);
}

#pragma endregion NodeInformation

#pragma region CoordinatorInformation

CoordinatorInformation* init_coordinator_information(HashTable* students, RingBuffer* ring_buffer, SinglyLinkedList* nodes, HANDLE exit_semaphore, HANDLE ring_buffer_semaphore, VoteList* votes) {

	CoordinatorInformation* coordinator_information = (CoordinatorInformation*)malloc(sizeof(CoordinatorInformation));
	if (IS_NULL(coordinator_information)) {
		return NULL;
	}

	coordinator_information->students = students;
	coordinator_information->ring_buffer = ring_buffer;
	coordinator_information->nodes = nodes;
	coordinator_information->exit_semaphore = exit_semaphore;
	coordinator_information->ring_buffer_semaphore = ring_buffer_semaphore;
	coordinator_information->votes = votes;

	return coordinator_information;
}

#pragma endregion CoordinatorInformation