#include "HashTable.h"

unsigned long hash(char* key, unsigned long HT_SIZE) {

    unsigned long hash = 5381;
    int c;

    while (c = *key++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash % HT_SIZE;
}

HashTable* ht_create(unsigned long table_size) {

    HashTable* hash_table = NULL;

    if (table_size <= 0) {
        return NULL;
    }

    hash_table = (HashTable*)malloc(sizeof(HashTable));

    if (hash_table == NULL) {
        return NULL;
    }

    hash_table->entries = (HashTableEntry**)malloc(sizeof(HashTableEntry*) * table_size);

    if (hash_table->entries == NULL) {
        return NULL;
    }

    hash_table->size = table_size;
    hash_table->counter = 0;

    for (size_t i = 0; i < table_size; i++) {
        hash_table->entries[i] = NULL;
    }

    InitializeCriticalSection(&hash_table->ht_cs);

    return hash_table;
}

HashTableEntry* ht_new_entry(char* key, Student student) {

    if (key == NULL) {
        return NULL;
    }

    HashTableEntry* ht_entry = NULL;

    ht_entry = (HashTableEntry*)malloc(sizeof(HashTableEntry));

    if (ht_entry == NULL) {
        return NULL;
    }

    ht_entry->key = (char*)malloc(strlen(key) + 1);

    if (ht_entry->key == NULL) {
        return NULL;
    }
    strcpy(ht_entry->key, key);

    ht_entry->student = (Student*)malloc(sizeof(Student));

    if (ht_entry->student == NULL) {
        return NULL;
    }

    fill_student(ht_entry->student, student.first_name, student.last_name, student.index);

    ht_entry->next_entry = NULL;

    return ht_entry;
}

bool ht_add(HashTable* hash_table, char* key, Student student) {

    EnterCriticalSection(&hash_table->ht_cs);

    if (hash_table == NULL || key == NULL) {
        LeaveCriticalSection(&hash_table->ht_cs);
        return false;
    }

    if (ht_find_by_key(hash_table, key)) {
        LeaveCriticalSection(&hash_table->ht_cs);
        return false;
    }

    unsigned long slot = hash(key, hash_table->size);

    HashTableEntry* new_entry = ht_new_entry(key, student);
    if (new_entry == NULL) {
        LeaveCriticalSection(&hash_table->ht_cs);
        return false;
    }

    if (hash_table->entries[slot] == NULL) {
        hash_table->entries[slot] = new_entry;
    }
    else {
        HashTableEntry* current = hash_table->entries[slot];

        while (current->next_entry != NULL) {
            current = current->next_entry;
        }

        current->next_entry = new_entry;
    }
    hash_table->counter++;

    LeaveCriticalSection(&hash_table->ht_cs);

    return true;
}

HashTableEntry* ht_get_by_key(HashTable* hash_table, char* key) {

    EnterCriticalSection(&hash_table->ht_cs);

    if (hash_table == NULL) {
        LeaveCriticalSection(&hash_table->ht_cs);
        return NULL;
    }

    unsigned int slot = hash(key, hash_table->size);

    HashTableEntry* current = hash_table->entries[slot];

    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            LeaveCriticalSection(&hash_table->ht_cs);
            return current;
        }

        current = current->next_entry;
    }

    LeaveCriticalSection(&hash_table->ht_cs);

    return NULL;
}

bool ht_find_by_key(HashTable* hash_table, char* key) {

    EnterCriticalSection(&hash_table->ht_cs);

    if (hash_table == NULL) {
        LeaveCriticalSection(&hash_table->ht_cs);
        return false;
    }

    unsigned int slot = hash(key, hash_table->size);

    HashTableEntry* current = hash_table->entries[slot];

    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            LeaveCriticalSection(&hash_table->ht_cs);
            return true;
        }

        current = current->next_entry;
    }

    LeaveCriticalSection(&hash_table->ht_cs);

    return false;
}

void ht_show(HashTable* hash_table) {

    EnterCriticalSection(&hash_table->ht_cs);

    if (hash_table == NULL) {
        printf("The hash table not found.");
        LeaveCriticalSection(&hash_table->ht_cs);
        return;
    }

    if (hash_table->counter == 0) {
        printf("The hash table is empty.");
        LeaveCriticalSection(&hash_table->ht_cs);
        return;
    }

    printf("-> HASH TABLE \n");
    for (size_t i = 0; i < hash_table->size; i++) {
        if (hash_table->entries[i] == NULL)
            continue;

        printf("\t Entry[%d]: ", i);

        HashTableEntry* current = hash_table->entries[i];

        while (current != NULL) {
            printf("<%s,<%s:%s:%s>> ", current->key
                                     , current->student->first_name
                                     , current->student->last_name
                                     , current->student->index);
            current = current->next_entry;
        }
        printf("\n");
    }

    LeaveCriticalSection(&hash_table->ht_cs);

}

void ht_free_entry(HashTableEntry* entry) {

    if (entry == NULL) {
        return;
    }

    HashTableEntry* current = entry;
    HashTableEntry* temp = NULL;

    while (current != NULL) {

        temp = current->next_entry;
        free(current->key);
        free(current->student->first_name);
        free(current->student->last_name);
        free(current->student->index);
        free(current->student);
        free(current);
        current = temp;
    }
}

void ht_free(HashTable* hash_table) {

    if (hash_table == NULL) {
        return;
    }

    for (size_t i = 0; i < hash_table->size; i++) {

        if (hash_table->entries[i] != NULL) {
            ht_free_entry(hash_table->entries[i]);
        }
    }

    free(hash_table->entries);
    DeleteCriticalSection(&hash_table->ht_cs);
    free(hash_table);
}