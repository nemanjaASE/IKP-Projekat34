#pragma once

#ifndef HANDLE_LIST_H
#define HANDLE_LIST_H

#define _CRT_SECURE_NO_WARNINGS 1 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ws2tcpip.h>

typedef struct node_handle_t {
	HANDLE thread_handle;
	struct node_handle_t* next_node;
} NodeHandle;

typedef struct handle_list_t {
	NodeHandle* head;
	unsigned int counter;
	CRITICAL_SECTION hl_cs;
} HandleList;

HandleList* hl_create();

NodeHandle* hl_new_node(HANDLE value);

bool hl_insert_first(HandleList* handle_list, HANDLE value);

bool hl_insert_last(HandleList* handle_list, HANDLE value);

bool hl_delete(HandleList* handle_list, HANDLE value);

void hl_free(HandleList* handle_list);

#endif
