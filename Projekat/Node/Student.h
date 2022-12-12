#pragma once

#ifndef STUDENT_H
#define STUDENT_H

#define _CRT_SECURE_NO_WARNINGS 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cstdint>

#define IS_NULL(a) (a == NULL)

typedef struct student_t {
	char* first_name;
	char* last_name;
	char* index;
} Student;

typedef struct header_t {
	uint8_t first_name_len;
	uint8_t last_name_len;
	uint8_t index_len;
} Header;

Student* create_student();

void fill_student(Student* student, char* first_name, char* last_name, char* index);

size_t fill_header(Student student, unsigned char* header);

char* serialize_student(Student* student);

void deserialize_student(Student* student, char* buffer, Header header);

void free_student(Student* student);

#endif STUDENT_H
