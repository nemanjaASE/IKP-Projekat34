#pragma once

#ifndef NODE_HELPER_H
#define NODE_HELPER_H

#include <stdlib.h>
#include <stdio.h>
#include "TCPUtils.h"
#include "SinglyLinkedList.h"
#include "Student.h"
#include "HashTable.h"

#define NODE_IP_ADDRESS "127.0.0.1"
#define PORT_MIN 30000

#pragma region Setup

bool nh_init_listen_socket(SOCKET* listen_socket, sockaddr_in* address, unsigned long port);

bool nh_init_connect_socket(SOCKET* connect_socket, sockaddr_in* address, unsigned long node_port);

unsigned long nh_port_number_input(char* msg);

#pragma endregion Setup


#pragma region IntegrityUpdate

bool nh_integrity_update(SinglyLinkedList* nodes, HashTable* students);

bool nh_fill_nodes(SinglyLinkedList* nodes, int number_of_nodes);

bool nh_broadcast_message(SinglyLinkedList* nodes);

unsigned long nh_receive_number_of_students(Node* first_node);

void nh_receive_students(Node* first_node, unsigned long number_of_students, HashTable* students);

#pragma endregion

#endif NODE_HELPER_H