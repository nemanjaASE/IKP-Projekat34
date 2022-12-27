#include "VoteList.h"

VoteList* vl_create() {

	VoteList* vote_list = NULL;

	vote_list = (VoteList*)malloc(sizeof(VoteList));

	if (vote_list == NULL) {
		return NULL;
	}

	vote_list->counter = 0;
	vote_list->head = NULL;

	InitializeCriticalSection(&vote_list->vl_cs);

	vote_list->vote_signal = CreateSemaphore(0, 0, 100, NULL);
	if (vote_list->vote_signal == NULL) {
		return NULL;
	}

	return vote_list;
}

Vote* vl_new_vote(int value) {

	Vote* new_vote = NULL;

	new_vote = (Vote*)malloc(sizeof(Vote));

	if (new_vote == NULL) {
		return NULL;
	}

	new_vote->value = value;
	new_vote->next_node = NULL;

	return new_vote;
}

bool vl_insert_first(VoteList* vote_list, int value) {

	if (vote_list == NULL) {
		return false;
	}

	EnterCriticalSection(&vote_list->vl_cs);

	Vote* new_vote = vl_new_vote(value);

	if (new_vote == NULL) {
		LeaveCriticalSection(&vote_list->vl_cs);
		return false;
	}

	if (vote_list->head == NULL) {
		vote_list->head = new_vote;
	}
	else {
		new_vote->next_node = vote_list->head;
		vote_list->head = new_vote;
	}

	vote_list->counter++;

	LeaveCriticalSection(&vote_list->vl_cs);

	return true;
}

bool vl_insert_last(VoteList* vote_list, int value) {

	if (vote_list == NULL) {
		return false;
	}

	EnterCriticalSection(&vote_list->vl_cs);

	Vote* new_vote = vl_new_vote(value);

	if (new_vote == NULL) {
		LeaveCriticalSection(&vote_list->vl_cs);
		return false;
	}

	if (vote_list->head == NULL) {
		vote_list->head = new_vote;
	}
	else {
		Vote* current = vote_list->head;

		while (current->next_node != NULL) {
			current = current->next_node;
		}

		current->next_node = new_vote;
	}

	vote_list->counter++;

	LeaveCriticalSection(&vote_list->vl_cs);

	return true;
}

int vl_get(VoteList* vote_list, int index) {

	if (vote_list == NULL || vote_list->counter < (unsigned int)index) {
		return 0;
	}

	EnterCriticalSection(&vote_list->vl_cs);

	Vote* target = vote_list->head;
	int i = 0;

	while (i < index) {

		target = target->next_node;
		i++;
	}

	LeaveCriticalSection(&vote_list->vl_cs);

	return target->value;
}

bool vl_delete_first(VoteList* vote_list) {

	if (vote_list == NULL) {
		return false;
	}

	EnterCriticalSection(&vote_list->vl_cs);

	Vote* head = vote_list->head;
	
	vote_list->head = head->next_node;

	free(head);
	
	vote_list->counter--;

	LeaveCriticalSection(&vote_list->vl_cs);

	return true;
}

bool vl_update(VoteList* vote_list, int index, int value) {

	if (vote_list == NULL || vote_list->counter < (unsigned int)index) {
		return false;
	}

	EnterCriticalSection(&vote_list->vl_cs);

	Vote* target = vote_list->head;
	int i = 0;

	while (i < index) {

		target = target->next_node;
		i++;
	}

	target->value = value; 

	LeaveCriticalSection(&vote_list->vl_cs);

	return false;
}

bool vl_check_votes(VoteList* vote_list) {

	if (vote_list == NULL) {
		return false;
	}

	EnterCriticalSection(&vote_list->vl_cs);

	Vote* current = vote_list->head;

	while (current != NULL) {

		if (current->value == 0) {
			LeaveCriticalSection(&vote_list->vl_cs);
			return false;
		}

		current = current->next_node;
	}

	LeaveCriticalSection(&vote_list->vl_cs);

	return true;
}

void vl_clear(VoteList* vote_list) {

	if (vote_list == NULL) {
		return;
	}

	EnterCriticalSection(&vote_list->vl_cs);

	Vote* current = vote_list->head;
	Vote* temp = NULL;

	while (current != NULL) {

		temp = current->next_node;
		free(current);

		current = temp;
	}

	vote_list->head = NULL;

	LeaveCriticalSection(&vote_list->vl_cs);
}

void vl_free(VoteList* vote_list) {

	if (vote_list == NULL) {
		return;
	}

	Vote* current = vote_list->head;
	Vote* temp = NULL;

	while (current != NULL) {

		temp = current->next_node;
		free(current);

		current = temp;
	}

	DeleteCriticalSection(&vote_list->vl_cs);

	CloseHandle(vote_list->vote_signal);

	free(vote_list);
}