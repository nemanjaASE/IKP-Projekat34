#pragma once

#ifndef CLIENT_HELPER_H
#define CLIENT_HELPER_H

#include "TCPUtils.h"
#include "Student.h"

#define PORT_MIN 30000
#define NODE_IP_ADDRESS "127.0.0.1"

#define FIRST_NAME_MAX 25
#define LAST_NAME_MAX 25
#define INDEX_MAX 15

bool ch_init_connect_socket(SOCKET* socket, sockaddr_in* address, unsigned long node_port);

unsigned long ch_port_number_input();

int ch_client_menu();

void ch_student_input(Student* student);

void ch_clear_newline(char* str);

int ch_send(SOCKET connect_socket, unsigned char* header, size_t header_size, char* buffer, size_t buffer_size);

#endif CLIENT_HELPER_H
