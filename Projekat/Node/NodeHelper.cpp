#include "NodeHelper.h"

#pragma region Setup

unsigned long nh_port_number_input(char* msg) {

	unsigned long port = 0;

	do {
		printf("- > Port for %s connections [%d >]: ", msg, PORT_MIN);
		scanf_s("%lu", &port);

		if (port <= PORT_MIN) {
			printf("- > The port number must be greater than %d\n", PORT_MIN);
		}

	} while (port <= PORT_MIN);

	return port;
}

bool nh_init_listen_socket(SOCKET* listen_socket, sockaddr_in* address, unsigned long port) {

	// Get sockaddr_in struct based on node port
	*address = get_server_socket_address_struct(port);

	// Create listen socket
	*listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (*listen_socket == INVALID_SOCKET) {
		printf("Creation listen socket error. %ld\n", WSAGetLastError());
		WSACleanup();

		return false;
	}

	// Bind listen socket with node address
	if (!bind(listen_socket, *address, sizeof(*address))) {
		return false;
	}

	// Put socket in non blocking mode
	if (!set_non_blocking_mode(listen_socket))
		return false;

	// Put socket in listening mode
	if (!set_listening_mode(listen_socket))
		return false;

	return true;
}

bool nh_init_connect_socket(SOCKET* connect_socket, sockaddr_in* address, unsigned long node_port) {

	// Create connect socket
	*connect_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (*connect_socket == INVALID_SOCKET) {
		printf("Creation connect socket error. %ld\n", WSAGetLastError());
		WSACleanup();

		return false;
	}

	// Get sockaddr_in struct based on node port
	*address = get_client_socket_address_struct(node_port, (char*)NODE_IP_ADDRESS);

	// Connect to server
	int i_result = connect(*connect_socket, (SOCKADDR*)&(*address), sizeof(*address));

	if (i_result == SOCKET_ERROR) {

		printf("Unable to connect to server. %ld\n", WSAGetLastError());
		closesocket(*connect_socket);
		WSACleanup();
		return false;
	}

	// Put socket in non blocking mode
	if (!set_non_blocking_mode(connect_socket))
		return false;

	return true;
}

#pragma endregion Setup

#pragma region IntegrityUpdate

bool nh_integrity_update(SinglyLinkedList* nodes, HashTable* students) {

	printf("First Node? [Y/N] ");

	char option;
	scanf_s("%c", &option);

	if (option == 'N' || option == 'n') {

		int number_of_nodes = -1;

		printf("Number of nodes: ");

		scanf_s("%d", &number_of_nodes);

		// Port input and connection establish

		if (!nh_fill_nodes(nodes, number_of_nodes)) {
			return false;
		}

		// Send message to other nodes

		if (!nh_broadcast_message(nodes)) {
			return false;
		}
		
		// Receive number of students

		Node* first_node = nodes->head;

		unsigned long students_num = nh_receive_number_of_students(first_node);

		// Receive students

		nh_receive_students(first_node, students_num, students);
	}

	return true;
}

bool nh_fill_nodes(SinglyLinkedList* nodes, int number_of_nodes) {

	unsigned long port = 0;
	int i = 0;

	while (i++ < number_of_nodes) {

		SOCKET connect_socket = INVALID_SOCKET;
		sockaddr_in server_address;

		port = nh_port_number_input((char*)"NODE");

		if (!nh_init_connect_socket(&connect_socket, &server_address, port)) {
			return false;
		}
		else {
			printf("[CONNECTED TO] '%s':'%lu'\n",
				inet_ntoa(server_address.sin_addr), ntohs(server_address.sin_port));

			sll_insert_first(nodes, connect_socket);
		}
	}

	return true;
}

bool nh_broadcast_message(SinglyLinkedList* nodes) {

	int i = 0;

	Node* current = nodes->head;

	while (current != NULL) {

		char message = '0';
		if (current == nodes->head) {
			message = '1';
		}

		select_function(current->node_socket, WRITE);

		int i_result = send(current->node_socket, (char*)&message, sizeof(char), 0);

		if (i_result == SOCKET_ERROR || i_result == 0)
		{
			i_result = shutdown(nodes->head->node_socket, SD_SEND);
			if (i_result == SOCKET_ERROR)
			{
				printf("Shutdown failed with error: %d\n", WSAGetLastError());
			}
			closesocket(nodes->head->node_socket);
			return false;
		}
		current = current->next_node;
	}

	return true;
}

unsigned long nh_receive_number_of_students(Node* first_node) {

	char* receive_buffer = (char*)malloc(sizeof(unsigned long));
	if (receive_buffer == NULL) {
		return 0;
	}
	unsigned int bytes_sent = 0;

	// Receive number of students

	do {
		select_function(first_node->node_socket, READ);

		int i_result = recv(first_node->node_socket, receive_buffer + bytes_sent, sizeof(unsigned long) - bytes_sent, 0);
		if (i_result == SOCKET_ERROR || i_result == 0)
		{
			i_result = shutdown(first_node->node_socket, SD_SEND);
			if (i_result == SOCKET_ERROR)
			{
				printf("Shutdown failed with error: %d\n", WSAGetLastError());
			}
			free(receive_buffer);
			closesocket(first_node->node_socket);
			return 0;
		}
		bytes_sent += i_result;
	} while (bytes_sent < sizeof(unsigned long));

	char* stop_string;
	unsigned long students_num = strtoul(receive_buffer, &stop_string, 10);
	free(receive_buffer);

	return students_num;
}

void nh_receive_students(Node* first_node, unsigned long number_of_students, HashTable* students) {

	unsigned int bytes_recieved = 0;
	int i_result = 0;
	int body_size = 0;
	int end = 0;

	Header header;
	int header_size = 3 * sizeof(uint8_t);
	Student* student = create_student();

	while (number_of_students > 0) {

		bytes_recieved = 0;
		body_size = 0;

		//Receive header

		do
		{
			select_function(first_node->node_socket, READ);
			i_result = recv(first_node->node_socket, (char*)&header + bytes_recieved, header_size - bytes_recieved, 0);
			if (i_result == SOCKET_ERROR || i_result == 0) {
				end = 1;
				break;
			}
			bytes_recieved += i_result;

		} while (bytes_recieved < header_size);

		if (end == 1) {
			break;
		}

		body_size = header.first_name_len + header.last_name_len + header.index_len;
		printf("The node sent payload message length: %d\n", body_size);

		bytes_recieved = 0;
		char* buffer = (char*)malloc(sizeof(char) * body_size + 1);

		if (IS_NULL(buffer)) {
			free(student);
			return;
		}

		// Receive body
		do
		{
			select_function(first_node->node_socket, READ);
			i_result = recv(first_node->node_socket, buffer + bytes_recieved, body_size - bytes_recieved, 0);
			if (i_result == SOCKET_ERROR || i_result == 0) {
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

		if (ht_add(students, student->index, *student)) {

			printf("The student '%s:%s:%s' successfully inserted.\n", student->first_name
				, student->last_name
				, student->index);
		}

		free(buffer);
		number_of_students--;
	}

	free_student(student);
}

#pragma endregion IntegrityUpdate