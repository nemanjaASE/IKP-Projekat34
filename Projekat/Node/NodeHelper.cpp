#include "NodeHelper.h"

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