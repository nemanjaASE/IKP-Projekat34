#include "HandleList.h"

HandleList* hl_create() {

	HandleList* handle_list = NULL;

	handle_list = (HandleList*)malloc(sizeof(HandleList));

	if (handle_list == NULL) {
		return NULL;
	}

	handle_list->counter = 0;
	handle_list->head = NULL;

	InitializeCriticalSection(&handle_list->hl_cs);

	return handle_list;
}

NodeHandle* hl_new_node(HANDLE value) {

	NodeHandle* new_node = NULL;

	new_node = (NodeHandle*)malloc(sizeof(NodeHandle));

	if (new_node == NULL) {
		return NULL;
	}

	new_node->thread_handle = value;
	new_node->next_node = NULL;

	return new_node;
}

bool hl_insert_first(HandleList* handle_list, HANDLE value) {

	if (handle_list == NULL) {
		return false;
	}

	EnterCriticalSection(&handle_list->hl_cs);

	NodeHandle* new_node = hl_new_node(value);

	if (new_node == NULL) {
		LeaveCriticalSection(&handle_list->hl_cs);
		return false;
	}

	if (handle_list->head == NULL) {
		handle_list->head = new_node;
	}
	else {
		new_node->next_node = handle_list->head;
		handle_list->head = new_node;
	}

	handle_list->counter++;

	LeaveCriticalSection(&handle_list->hl_cs);

	return true;
}

bool hl_insert_last(HandleList* handle_list, HANDLE value) {

	if (handle_list == NULL) {
		return false;
	}

	EnterCriticalSection(&handle_list->hl_cs);

	NodeHandle* new_node = hl_new_node(value);

	if (new_node == NULL) {
		LeaveCriticalSection(&handle_list->hl_cs);
		return false;
	}

	if (handle_list->head == NULL) {
		handle_list->head = new_node;
	}
	else {
		NodeHandle* current = handle_list->head;

		while (current->next_node != NULL) {
			current = current->next_node;
		}

		current->next_node = new_node;
	}

	handle_list->counter++;

	LeaveCriticalSection(&handle_list->hl_cs);

	return true;
}

bool hl_delete(HandleList* handle_list, HANDLE value) {

	if (handle_list == NULL) {
		return false;
	}

	EnterCriticalSection(&handle_list->hl_cs);

	NodeHandle* current = handle_list->head;
	NodeHandle* prev = NULL;

	if (current != NULL && current->thread_handle == value) {

		handle_list->head = current->next_node;
		free(current);
		handle_list->counter--;
		LeaveCriticalSection(&handle_list->hl_cs);
		return true;
	}

	while (current != NULL && current->thread_handle != value) {
		prev = current;
		current = current->next_node;
	}

	if (current == NULL) {
		LeaveCriticalSection(&handle_list->hl_cs);
		return false;
	}

	prev->next_node = current->next_node;
	free(current);

	handle_list->counter--;

	LeaveCriticalSection(&handle_list->hl_cs);

	return true;
}

void hl_free(HandleList* handle_list) {

	if (handle_list == NULL) {
		return;
	}

	NodeHandle* current = handle_list->head;
	NodeHandle* temp = NULL;

	while (current != NULL) {

		temp = current->next_node;
		CloseHandle(current->thread_handle);
		free(current);

		current = temp;
	}

	DeleteCriticalSection(&handle_list->hl_cs);

	free(handle_list);
}