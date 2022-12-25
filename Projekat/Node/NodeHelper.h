#pragma once

#ifndef NODE_HELPER_H
#define NODE_HELPER_H

#include <stdlib.h>
#include <stdio.h>
#include "TCPUtils.h"
#include "SinglyLinkedList.h"
#include "Student.h"
#include "HashTable.h"
#include "HandleList.h"

#define NODE_IP_ADDRESS "127.0.0.1"
#define PORT_MIN 10000

#define FIRST_NAME_MAX 25
#define LAST_NAME_MAX 25
#define INDEX_MAX 15

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

#pragma region Threads

void wait_for_all_threads(HandleList* handles);

#pragma endregion Threads

#pragma region Shutdown

void graceful_exit(Node* head);

#pragma endregion Shutdown

#endif NODE_HELPER_H