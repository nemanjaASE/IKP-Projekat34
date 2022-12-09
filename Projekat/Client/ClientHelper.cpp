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