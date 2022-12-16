#include "Student.h"

Student* create_student() {
	Student* student = NULL;

	student = (Student*)malloc(sizeof(Student));

	if (IS_NULL(student)) {
		printf("There is not enough memory for the student struct.\n");
		return NULL;
	}

	student->first_name = NULL;
	student->last_name = NULL;
	student->index = NULL;

	return student;
}

void fill_student(Student* student, char* first_name, char* last_name, char* index) {

	if (IS_NULL(student) || IS_NULL(first_name) || IS_NULL(last_name) || IS_NULL(index)) {
		return;
	}

	student->first_name = (char*)calloc(strlen(first_name) + 1, sizeof(char));
	if (IS_NULL(student->first_name)) {
		printf("There is not enough memory for the student first name.\n");
		return;
	}

	student->last_name = (char*)calloc(strlen(last_name) + 1, sizeof(char));
	if (IS_NULL(student->last_name)) {
		printf("There is not enough memory for the student last name.\n");
		free(student->first_name);
		return;
	}

	student->index = (char*)calloc(strlen(index) + 1, sizeof(char));
	if (IS_NULL(student->index)) {
		printf("There is not enough memory for the student index.\n");
		free(student->first_name);
		free(student->index);
		return;
	}

	strcpy(student->first_name, first_name);
	strcpy(student->last_name, last_name);
	strcpy(student->index, index);
}

void update_student(Student* student, char* first_name, char* last_name, char* index) {

	if (IS_NULL(student) || IS_NULL(first_name) || IS_NULL(last_name) || IS_NULL(index)) {
		return;
	}

	char* temp = NULL;

	temp = (char*)realloc(student->first_name, sizeof(char) * (strlen(first_name) + 1));
	if (IS_NULL(temp)) {
		printf("There is not enough memory for the student first name.\n");
		return;
	}
	student->first_name = temp;
	temp = NULL;

	temp = (char*)realloc(student->last_name, sizeof(char) * (strlen(last_name) + 1));
	if (IS_NULL(temp)) {
		printf("There is not enough memory for the student last name.\n");
		free(student->first_name);
		return;
	}
	student->last_name = temp;
	temp = NULL;

	temp = (char*)realloc(student->index, sizeof(char) * (strlen(index) + 1));
	if (IS_NULL(temp)) {
		printf("There is not enough memory for the student index.\n");
		free(student->first_name);
		free(student->index);
		return;
	}
	student->index = temp;
	temp = NULL;

	strcpy(student->first_name, first_name);
	strcpy(student->last_name, last_name);
	strcpy(student->index, index);
}

void free_student(Student* student) {

	if (IS_NULL(student)) {
		return;
	}

	free(student->first_name);
	free(student->last_name);
	free(student->index);
	free(student);
}

size_t fill_header(Student student, unsigned char* header) {

	Header header_struct;

	header_struct.first_name_len = (uint8_t)strlen(student.first_name);
	header_struct.last_name_len = (uint8_t)strlen(student.last_name);
	header_struct.index_len = (uint8_t)strlen(student.index);

	memcpy(header, (const unsigned char*)&header_struct, sizeof(Header));

	return header_struct.first_name_len + header_struct.last_name_len + header_struct.index_len;
}

char* serialize_student(Student* student) {

	char* buffer = NULL;
	int first_name_len = strlen(student->first_name);
	int last_name_len = strlen(student->last_name);
	int index_len = strlen(student->index);

	int msg_len = first_name_len + last_name_len + index_len;

	buffer = (char*)malloc(sizeof(char) * (msg_len + 1));
	if (IS_NULL(buffer)) {
		printf("Not enough memory.");
		return NULL;
	}

	memcpy(buffer, student->first_name, first_name_len);
	memcpy(buffer + first_name_len, student->last_name, last_name_len);
	memcpy(buffer + first_name_len + last_name_len, student->index, index_len);

	buffer[msg_len] = '\0';

	return buffer;
}

void deserialize_student(Student* student, char* buffer, Header header) {

	char* first_name = (char*)calloc(header.first_name_len + 1, sizeof(char));
	if (IS_NULL(first_name)) {
		return;
	}

	char* last_name = (char*)calloc(header.last_name_len + 1, sizeof(char));
	if (IS_NULL(last_name)) {
		return;
	}

	char* index = (char*)calloc(header.index_len + 1, sizeof(char));
	if (IS_NULL(index)) {
		return;
	}

	memcpy(first_name, (const char*)buffer, header.first_name_len);
	memcpy(last_name, (const char*)buffer + header.first_name_len, header.last_name_len);
	memcpy(index, (const char*)buffer + header.first_name_len + header.last_name_len, header.index_len);

	if (IS_NULL(student->first_name) || IS_NULL(student->last_name) || IS_NULL(student->index)) {
		fill_student(student, first_name, last_name, index);
	}
	else {
		update_student(student, first_name, last_name, index);
	}

	free(first_name);
	free(last_name);
	free(index);
}