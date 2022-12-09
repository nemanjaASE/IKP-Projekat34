#include "NodeHelper.h"

int main() {

	if (!initialize_windows_sockets()) {
		return -1;
	}

	SOCKET client_listen_socket = INVALID_SOCKET;
	SOCKET node_listen_socket = INVALID_SOCKET;
	sockaddr_in client_address;
	sockaddr_in node_address;

	unsigned long client_port = 0;
	unsigned long node_port = 0;
	
	client_port = nh_port_number_input((char*)"CLIENT");
	node_port = nh_port_number_input((char*)"NODE");

	system("cls");

	if (!nh_init_listen_socket(&client_listen_socket, &client_address, client_port)) {
		return -1;
	}
	else {
		printf("------------------------------------------\n");
		printf("[CLIENT LISTEN SOCKET] '%s':'%lu' |\n",
			inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
		printf("------------------------------------------\n");
	}

	if (!nh_init_listen_socket(&node_listen_socket, &node_address, node_port)) {
		return -1;
	}
	else {
		printf("[NODE LISTEN SOCKET] '%s':'%lu'   |\n",
			inet_ntoa(node_address.sin_addr), ntohs(node_address.sin_port));
		printf("------------------------------------------\n");
	}
	
	fd_set read_fds;
	FD_ZERO(&read_fds);

	timeval timeVal;
	timeVal.tv_sec = 2;
	timeVal.tv_usec = 0;

	int client_counter = 0;
	int i_result = 0;
	while (1) {

		FD_ZERO(&read_fds);

		if (client_counter == 0) {
			FD_SET(client_listen_socket, &read_fds);
		}

		FD_SET(node_listen_socket, &read_fds);

		i_result = select(0, &read_fds, NULL, NULL, &timeVal);

		if (i_result == SOCKET_ERROR) {
			printf("Error %d \n", WSAGetLastError());
			break;
		}
		else if (i_result == 0) {
			continue;
		}
		else {

			if (FD_ISSET(client_listen_socket, &read_fds)) {
				// Client connected
				printf("New client!!\n");
				client_counter = 1;
			}

			if (FD_ISSET(node_listen_socket, &read_fds)) {
				// Node connected
			}
		}
	}

	closesocket(client_listen_socket);
	closesocket(node_listen_socket);
	WSACleanup();

	return 0;
}