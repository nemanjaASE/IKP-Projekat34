#pragma once

#ifndef NODE_HELPER_H
#define NODE_HELPER_H

#include <stdlib.h>
#include <stdio.h>
#include "TCPUtils.h"

#define PORT_MIN 30000

bool nh_init_listen_socket(SOCKET* listen_socket, sockaddr_in* address, unsigned long port);
unsigned long nh_port_number_input(char* msg);

#endif NODE_HELPER_H