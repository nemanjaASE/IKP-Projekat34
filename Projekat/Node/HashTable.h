#pragma once

#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#define _CRT_SECURE_NO_WARNINGS 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Student.h"

typedef struct hash_table_entry_t {
	char* key;
	Student* student;
	struct hash_table_entry_t* next_entry;
} HashTableEntry;

typedef struct hash_table_t {
	HashTableEntry** entries;
	unsigned long size;
	unsigned long counter;
} HashTable;

unsigned long hash(char* key, unsigned long HT_SIZE);

HashTable* ht_create(unsigned long table_size);

HashTableEntry* ht_new_entry(char* key, Student student);

bool ht_add(HashTable* hash_table, char* key, Student student);

HashTableEntry* ht_get_by_key(HashTable* hash_table, char* key);

bool ht_find_by_key(HashTable* hash_table, char* key);

void ht_show(HashTable* hash_table);

void ht_free_entry(HashTableEntry* entry);

void ht_free(HashTable* hash_table);

#endif
