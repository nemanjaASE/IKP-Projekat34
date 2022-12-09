#include "ClientHelper.h"

int main() {

	if (!initialize_windows_sockets()) {
		return -1;
	}

	SOCKET connect_socket = INVALID_SOCKET;
	sockaddr_in server_address;
	unsigned long node_port = 0;

	node_port = ch_port_number_input();
	
	if (!ch_init_connect_socket(&connect_socket, &server_address, node_port)) {
		return -1;
	}

	do {

	
	} while (1);


	closesocket(connect_socket);
	WSACleanup();

	return 0;
}