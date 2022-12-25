#include "SinglyLinkedList.h"

SinglyLinkedList* sll_create() {

	SinglyLinkedList* singly_linked_list = NULL;

	singly_linked_list = (SinglyLinkedList*)malloc(sizeof(SinglyLinkedList));

	if (singly_linked_list == NULL) {
		return NULL;
	}

	singly_linked_list->counter = 0;
	singly_linked_list->head = NULL;

	InitializeCriticalSection(&singly_linked_list->sll_cs);

	return singly_linked_list;
}

Node* sll_new_node(SOCKET value) {

	Node* new_node = NULL;

	new_node = (Node*)malloc(sizeof(Node));

	if (new_node == NULL) {
		return NULL;
	}

	new_node->node_socket = value;
	new_node->next_node = NULL;

	return new_node;
}

bool sll_insert_first(SinglyLinkedList* singly_linked_list, SOCKET value) {

	if (singly_linked_list == NULL) {
		return false;
	}

	EnterCriticalSection(&singly_linked_list->sll_cs);

	Node* new_node = sll_new_node(value);

	if (new_node == NULL) {
		LeaveCriticalSection(&singly_linked_list->sll_cs);
		return false;
	}

	if (singly_linked_list->head == NULL) {
		singly_linked_list->head = new_node;
	}
	else {
		new_node->next_node = singly_linked_list->head;
		singly_linked_list->head = new_node;
	}

	singly_linked_list->counter++;

	LeaveCriticalSection(&singly_linked_list->sll_cs);

	return true;
}

bool sll_insert_last(SinglyLinkedList* singly_linked_list, SOCKET value) {

	if (singly_linked_list == NULL) {
		return false;
	}

	EnterCriticalSection(&singly_linked_list->sll_cs);

	Node* new_node = sll_new_node(value);

	if (new_node == NULL) {
		LeaveCriticalSection(&singly_linked_list->sll_cs);
		return false;
	}

	if (singly_linked_list->head == NULL) {
		singly_linked_list->head = new_node;
	}
	else {
		Node* current = singly_linked_list->head;

		while (current->next_node != NULL) {
			current = current->next_node;
		}

		current->next_node = new_node;
	}

	singly_linked_list->counter++;

	LeaveCriticalSection(&singly_linked_list->sll_cs);

	return true;
}

bool sll_delete(SinglyLinkedList* singly_linked_list, SOCKET value) {

	if (singly_linked_list == NULL) {
		return false;
	}

	EnterCriticalSection(&singly_linked_list->sll_cs);

	Node* current = singly_linked_list->head;
	Node* prev = NULL;

	if (current != NULL && current->node_socket == value) {

		singly_linked_list->head = current->next_node;
		free(current);
		singly_linked_list->counter--;
		LeaveCriticalSection(&singly_linked_list->sll_cs);
		return true;
	}

	while (current != NULL && current->node_socket != value) {
		prev = current;
		current = current->next_node;
	}

	if (current == NULL) {
		LeaveCriticalSection(&singly_linked_list->sll_cs);
		return false;
	}

	prev->next_node = current->next_node;
	free(current);

	singly_linked_list->counter--;

	LeaveCriticalSection(&singly_linked_list->sll_cs);

	return true;
}

void sll_show(SinglyLinkedList* singly_linked_list) {

	if (singly_linked_list == NULL) {
		printf("The singly linked list not found. \n");
		return;
	}

	EnterCriticalSection(&singly_linked_list->sll_cs);

	if (singly_linked_list->counter == 0) {
		printf("The singly linked list is empty. \n");
		LeaveCriticalSection(&singly_linked_list->sll_cs);
		return;
	}

	Node* current = singly_linked_list->head;

	while (current != NULL) {

		printf("<%llu> ", current->node_socket);

		current = current->next_node;
	}

	printf("\n");

	LeaveCriticalSection(&singly_linked_list->sll_cs);
}

void sll_free(SinglyLinkedList* singly_linked_list) {
	if (singly_linked_list == NULL) {
		return;
	}

	Node* current = singly_linked_list->head;
	Node* temp = NULL;

	while (current != NULL) {
		temp = current->next_node;
		closesocket(current->node_socket);
		free(current);

		current = temp;
	}

	DeleteCriticalSection(&singly_linked_list->sll_cs);

	free(singly_linked_list);
}