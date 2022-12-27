  #include "ClientHelper.h"

unsigned long ch_port_number_input() {

	unsigned long port = 0;

	do {
		printf("- > Node port: [%d >]: ", PORT_MIN);
		scanf_s("%lu", &port);

		if (port <= PORT_MIN) {
			printf("- > The port number must be greater than %d\n", PORT_MIN);
		}

	} while (port <= PORT_MIN);

	fgetc(stdin);

	return port;
}

bool ch_init_connect_socket(SOCKET* connect_socket, sockaddr_in* address, unsigned long node_port) {

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

int ch_client_menu() {
	int option = 1;

	printf("\n");
	printf("\t1. New student\n");
	printf("\t2. Exit\n");
	printf("-> ");
	do {
		scanf_s("%d", &option);

		if (option <= 0 && option > 2) {
			printf("Unknown command.\n");
			printf("-> ");
		}
	} while (option <= 0 && option > 2);

	fgetc(stdin);

	return option;
}

void ch_student_input(Student* student) {

	char first_name[FIRST_NAME_MAX];
	char last_name[LAST_NAME_MAX];
	char index[INDEX_MAX];

	printf("\n\tFirst name [<%d]: ", FIRST_NAME_MAX);
	if (fgets(first_name, FIRST_NAME_MAX, stdin)) {
		ch_clear_newline(first_name);
	}

	printf("\n\tLast name [<%d]: ", LAST_NAME_MAX);
	if (fgets(last_name, LAST_NAME_MAX, stdin)) {
		ch_clear_newline(last_name);
	}

	printf("\n\tIndex [<%d]: ", INDEX_MAX);
	if (fgets(index, INDEX_MAX, stdin)) {
		ch_clear_newline(index);
	}

	if (IS_NULL(student->first_name) && IS_NULL(student->last_name) && IS_NULL(student->index)) {

		fill_student(student, first_name, last_name, index);
	}
	else
	{
		update_student(student, first_name, last_name, index);
	}
}

void ch_clear_newline(char* str) {

	char* p;
	if (p = strchr(str, '\n')) { //check exist newline
		*p = 0;
	}
	else {
		scanf_s("%*[^\n]");
		scanf_s("%*c"); //clear upto newline
	}
}

int ch_send(SOCKET connect_socket, unsigned char* header, size_t header_size, char* buffer, size_t buffer_size) {

	unsigned int bytes_sent = 0;
	int i_result = 0;
	int end = 0;
	// Send header
	do {
		select_function(connect_socket, WRITE);

		i_result = send(connect_socket, (char*)header + bytes_sent, (unsigned int)header_size - bytes_sent, 0);

		if (i_result == SOCKET_ERROR || i_result == 0)
		{
			i_result = shutdown(connect_socket, SD_SEND);
			if (i_result == SOCKET_ERROR)
			{
				printf("Shutdown failed with error: %d\n", WSAGetLastError());
			}
			end = 1;
			break;
		}

		bytes_sent += i_result;

	} while (bytes_sent < header_size);

	// Send body

	bytes_sent = 0;
	

	do {
		select_function(connect_socket, WRITE);

		i_result = send(connect_socket, buffer + bytes_sent, (unsigned int)buffer_size - bytes_sent, 0);

		if (i_result == SOCKET_ERROR || i_result == 0)
		{
			i_result = shutdown(connect_socket, SD_SEND);
			if (i_result == SOCKET_ERROR)
			{
				printf("Shutdown failed with error: %d\n", WSAGetLastError());
			}
			end = 1;
			break;
		}

		bytes_sent += i_result;

	} while (bytes_sent < (unsigned int)buffer_size);

	return end;
}