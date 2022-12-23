#include "ClientHelper.h"
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

int main() {

	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	if (!initialize_windows_sockets()) {
		return -1;
	}

	SOCKET connect_socket = INVALID_SOCKET;
	sockaddr_in server_address;
	unsigned long node_port = 0;

	node_port = ch_port_number_input();
	
	system("cls");

	if (!ch_init_connect_socket(&connect_socket, &server_address, node_port)) {
		return -1;
	}
	else {
		printf("--------------------------------------\n");
		printf("[CONNECTED TO] '%s':'%lu'   |\n",
			inet_ntoa(server_address.sin_addr), ntohs(server_address.sin_port));
		printf("--------------------------------------\n");
	}

	Student* student = create_student();
	int option = -1;
	char* body = NULL;
	unsigned char* header = (unsigned char*)malloc(sizeof(Header));

	do {
		option = ch_client_menu();

		if (option == 1) {

			ch_student_input(student);

			size_t body_len = fill_header(*student, header);

			//for (int i = 0; i < sizeof(uint8_t) * 3; i++)
				//printf("%02X ", header[i]);
			//printf("\n");

			body = serialize_student(student);

			if (ch_send(connect_socket, header, 3 * sizeof(uint8_t), body, body_len) == 1)
			{
				free(body);
				break;
			}

			free(body);
		}
		if (option == 2) {
			break;
		}
		
	} while (1);

	shutdown(connect_socket, SD_BOTH);

	free(header);
	free_student(student);
	closesocket(connect_socket);
	WSACleanup();

	printf("\n\tClient terminated.");

	return 0;
}