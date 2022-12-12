#include "SinglyLinkedList.h"

SinglyLinkedList* sll_create() {

	SinglyLinkedList* singly_linked_list = NULL;

	singly_linked_list = (SinglyLinkedList*)malloc(sizeof(SinglyLinkedList));

	if (singly_linked_list == NULL) {
		return NULL;
	}

	singly_linked_list->counter = 0;
	singly_linked_list->head = NULL;

	return singly_linked_list;
}

Node* sll_new_node(char* value) {

	if (value == NULL) {
		return NULL;
	}

	Node* new_node = NULL;

	new_node = (Node*)malloc(sizeof(Node));

	if (new_node == NULL) {
		return NULL;
	}

	new_node->value = (char*)malloc(strlen(value) + 1);

	if (new_node->value == NULL) {
		return NULL;
	}

	strcpy(new_node->value, value);
	new_node->next_node = NULL;

	return new_node;
}

bool sll_insert_first(SinglyLinkedList* singly_linked_list, char* value) {

	if (singly_linked_list == NULL || value == NULL) {
		return false;
	}

	Node* new_node = sll_new_node(value);

	if (new_node == NULL) {
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

	return true;
}

bool sll_insert_last(SinglyLinkedList* singly_linked_list, char* value) {

	if (singly_linked_list == NULL || value == NULL) {
		return false;
	}

	Node* new_node = sll_new_node(value);

	if (new_node == NULL) {
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

	return true;
}

void sll_show(SinglyLinkedList* singly_linked_list) {

	if (singly_linked_list == NULL) {
		printf("The singly linked list not found. \n");
		return;
	}

	if (singly_linked_list->counter == 0) {
		printf("The singly linked list is empty. \n");
		return;
	}

	Node* current = singly_linked_list->head;

	while (current != NULL) {

		printf("<%s> ", current->value);

		current = current->next_node;
	}

	printf("\n");
}

void sll_free(SinglyLinkedList* singly_linked_list) {
	if (singly_linked_list == NULL || singly_linked_list->head == NULL) {
		return;
	}

	Node* current = singly_linked_list->head;
	Node* temp = NULL;

	while (current != NULL) {
		temp = current->next_node;

		free(current->value);
		free(current);

		current = temp;
	}

	free(singly_linked_list);
}