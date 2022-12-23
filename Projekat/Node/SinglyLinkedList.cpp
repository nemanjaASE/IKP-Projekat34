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

Node* sll_new_node(NetworkNode value) {

	Node* new_node = NULL;

	new_node = (Node*)malloc(sizeof(Node));

	if (new_node == NULL) {
		return NULL;
	}

	new_node->value.node_socket = value.node_socket;
	new_node->value.port = value.port;
	new_node->next_node = NULL;

	return new_node;
}

bool sll_insert_first(SinglyLinkedList* singly_linked_list, NetworkNode value) {

	EnterCriticalSection(&singly_linked_list->sll_cs);

	if (singly_linked_list == NULL) {
		LeaveCriticalSection(&singly_linked_list->sll_cs);
		return false;
	}

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

bool sll_insert_last(SinglyLinkedList* singly_linked_list, NetworkNode value) {

	EnterCriticalSection(&singly_linked_list->sll_cs);

	if (singly_linked_list == NULL) {
		LeaveCriticalSection(&singly_linked_list->sll_cs);
		return false;
	}

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

void sll_show(SinglyLinkedList* singly_linked_list) {

	EnterCriticalSection(&singly_linked_list->sll_cs);

	if (singly_linked_list == NULL) {
		printf("The singly linked list not found. \n");
		LeaveCriticalSection(&singly_linked_list->sll_cs);
		return;
	}

	if (singly_linked_list->counter == 0) {
		printf("The singly linked list is empty. \n");
		LeaveCriticalSection(&singly_linked_list->sll_cs);
		return;
	}

	Node* current = singly_linked_list->head;

	while (current != NULL) {

		printf("<%s> ", current->value);

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
		free(current);

		current = temp;
	}

	DeleteCriticalSection(&singly_linked_list->sll_cs);

	free(singly_linked_list);
}