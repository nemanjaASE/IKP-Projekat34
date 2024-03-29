#pragma once

#ifndef SINGLY_LINKED_LIST_H
#define SINGLY_LINKED_LIST_H

#define _CRT_SECURE_NO_WARNINGS 1 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ws2tcpip.h>

typedef struct node_t {
	SOCKET node_socket;
	struct node_t* next_node;
} Node;

typedef struct singly_linked_list_t {
	Node* head;
	unsigned int counter;
	CRITICAL_SECTION sll_cs;
} SinglyLinkedList;

SinglyLinkedList* sll_create();

Node* sll_new_node(SOCKET value);

bool sll_insert_first(SinglyLinkedList* singly_linked_list, SOCKET value);

bool sll_insert_last(SinglyLinkedList* singly_linked_list, SOCKET value);

bool sll_delete(SinglyLinkedList* singly_linked_list, SOCKET value);

void sll_show(SinglyLinkedList* singly_linked_list);

void sll_free(SinglyLinkedList* singly_linked_list);

#endif
