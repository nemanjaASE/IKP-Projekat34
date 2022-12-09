#pragma once

#ifndef CLIENT_HELPER_H
#define CLIENT_HELPER_H

#include "TCPUtils.h"

#define PORT_MIN 30000
#define NODE_IP_ADDRESS "127.0.0.1"

bool ch_init_connect_socket(SOCKET* socket, sockaddr_in* address, unsigned long node_port);

unsigned long ch_port_number_input();

void ch_student_input();
#endif CLIENT_HELPER_H
