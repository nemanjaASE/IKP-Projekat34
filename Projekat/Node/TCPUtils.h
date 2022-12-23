#pragma once

#ifndef __TCP_UTILS__
#define __TCP_UTILS__

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#include <stdlib.h>
#include <conio.h>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

typedef enum select_option_t {
	READ = 0,
	WRITE
} SelectOption;

bool initialize_windows_sockets();

sockaddr_in get_server_socket_address_struct(unsigned long port);

sockaddr_in get_client_socket_address_struct(unsigned long port,char* node_ip_address);

bool bind(SOCKET* listen_socket, sockaddr_in server_address, int size);

bool set_non_blocking_mode(SOCKET* socket);

bool set_listening_mode(SOCKET* listen_socket);

void select_function(SOCKET socket, SelectOption option, HANDLE exit_signal);

void select_function(SOCKET socket, SelectOption option);

SOCKET accept_new_socket(SOCKET listenSocket);

#endif // __TCP_UTILS__
