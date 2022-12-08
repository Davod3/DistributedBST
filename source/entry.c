// Ficheiro desenvolvido pelo grupo 7, cujos elementos são:
//   - André Dias    , nº 55314
//   - David Pereira , nº 56361
//   - Miguel Cut    , nº 56339

#include <entry.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

struct entry_t *entry_create(char *key, struct data_t *data) {
    
    struct entry_t* temp = (struct entry_t*) malloc(sizeof(struct entry_t));

    temp->key = key;
    temp->value = data;

    return temp;
}

void entry_destroy(struct entry_t *entry) {
    
    if(entry != NULL){
        data_destroy(entry->value);
        free(entry->key);
        free(entry);
    }
}

struct entry_t *entry_dup(struct entry_t *entry) {
    
    struct entry_t* temp = (struct entry_t*) malloc(sizeof(struct entry_t));
    temp->key = (char*) malloc(strlen(entry->key)*sizeof(char)+1);
    strcpy(temp->key,entry->key);
    temp->value = data_dup(entry->value);

    return temp;
}

void entry_replace(struct entry_t *entry, char *new_key, struct data_t *new_value) {
    data_destroy(entry->value);
    free(entry->key);

    entry->key = new_key;
    entry->value = new_value;
}

int entry_compare(struct entry_t *entry1, struct entry_t *entry2) {
    int temp;

    if (strcmp(entry1->key, entry2->key) > 0) {
        temp = 1;
    }
    else if (strcmp(entry1->key, entry2->key) < 0) {
        temp = -1;
    }
    else {
        temp = 0;
    }

    return temp;
}