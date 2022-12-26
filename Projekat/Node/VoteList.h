#pragma once

#ifndef VOTE_LIST_H
#define VOTE_LIST_H

#define _CRT_SECURE_NO_WARNINGS 1 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ws2tcpip.h>

typedef struct vote_t {
	int value;
	struct vote_t* next_node;
} Vote;

typedef struct vote_list_t {
	Vote* head;
	unsigned int counter;
	CRITICAL_SECTION vl_cs;
	HANDLE vote_signal;
} VoteList;

VoteList* vl_create();

Vote* vl_new_vote(int value);

bool vl_insert_first(VoteList* vote_list, int value);

bool vl_insert_last(VoteList* vote_list, int value);

int vl_get(VoteList* vote_list, int index);

bool vl_delete_first(VoteList* vote_list);

bool vl_update(VoteList* vote_list, int index, int value);

bool vl_check_votes(VoteList* vote_list);

void vl_clear(VoteList* vote_list);

void vl_free(VoteList* vote_list);

#endif

