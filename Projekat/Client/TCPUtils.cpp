#include "TCPUtils.h"

bool initialize_windows_sockets() {

	WSAData wsaData;
	// Initialize windows sockets library for this process
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {

		printf("WSAStartup failed with error %d \n", WSAGetLastError());
		return false;
	}
	return true;
}

sockaddr_in get_server_socket_address_struct(unsigned long port) {

	sockaddr_in serverAddress;
	int size = sizeof(serverAddress);
	memset((char*)&serverAddress, 0, size);

	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = INADDR_ANY;
	serverAddress.sin_port = htons(port);

	return serverAddress;
}

sockaddr_in get_client_socket_address_struct(unsigned long port, char* node_ip_address) {

	sockaddr_in server_address;
	int size = sizeof(server_address);
	memset((char*)&server_address, 0, size);

	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = inet_addr(node_ip_address);
	server_address.sin_port = htons(port);		

	return server_address;
}

bool bind(SOCKET* listen_socket, sockaddr_in server_address, int size) {

	int i_result = bind(*listen_socket, (SOCKADDR*)&server_address, size);

	if (i_result == SOCKET_ERROR) {
		printf("Bind failed with error: %d \n", WSAGetLastError());
		closesocket(*listen_socket);
		WSACleanup();
		return false;
	}
	return true;
}

bool set_non_blocking_mode(SOCKET* socket) {

	unsigned long mode = 1;
	int i_result = ioctlsocket(*socket, FIONBIO, &mode);

	if (i_result == SOCKET_ERROR) {
		printf("Ioctlsocket error. \n");
		closesocket(*socket);
		WSACleanup();

		return false;
	}

	return true;
}

bool set_listening_mode(SOCKET* listen_socket) {

	int i_result = listen(*listen_socket, SOMAXCONN);

	if (i_result == SOCKET_ERROR) {
		printf("Listen error. \n");
		closesocket(*listen_socket);
		WSACleanup();

		return false;
	}

	return true;
}

void select_function(SOCKET socket, SelectOption option) {

	FD_SET set;
	timeval time_val;

	time_val.tv_sec = 0;
	time_val.tv_usec = 0;

	int i_result = 0;

	while (1)
	{
		FD_ZERO(&set);
		FD_SET(socket, &set);

		if (option == READ)
			i_result = select(0, &set, NULL, NULL, &time_val);
		else if (option == WRITE)
			i_result = select(0, NULL, &set, NULL, &time_val);
		else
		{
			printf("\nUnknown SelectOption!\n");
			return;
		}


		if (i_result == 0) {
			continue;
		}
		else if (i_result == SOCKET_ERROR) {
			printf("\nError occured in select function.. %d\n", WSAGetLastError());
		}
		else {
			return;
		}
	}
}

SOCKET accept_new_socket(SOCKET listen_socket) {

	SOCKET accepted_socket = accept(listen_socket, NULL, NULL);

	if (accepted_socket == INVALID_SOCKET)
	{
		printf("Accept failed with error: %d\n", WSAGetLastError());
		closesocket(listen_socket);
		WSACleanup();
		return NULL;
	}

	return accepted_socket;
}