// Ficheiro desenvolvido pelo grupo 7, cujos elementos são:
//   - André Dias    , nº 55314
//   - David Pereira , nº 56361
//   - Miguel Cut    , nº 56339

#include <data.h>
#include <stdlib.h>
#include <string.h>

struct data_t *data_create(int size){

    if(size > 0){
        struct data_t* temp = (struct data_t*) malloc(sizeof(struct data_t)); //possible seg fault
        temp->datasize = size;
        temp->data = malloc(size);
        return temp;
    }

    return NULL;
}

struct data_t *data_create2(int size, void *data){
    
    if(size > 0 && data != NULL){
        struct data_t* temp = (struct data_t*) malloc(sizeof(struct data_t));
        temp->datasize = size;
        temp->data = data;
        return temp;
    } 

    return NULL;
}

void data_destroy(struct data_t *data){
    
    if(data != NULL){
        free(data->data);
        free(data);
    }
}

struct data_t *data_dup(struct data_t *data){
    
    if(data != NULL && data->datasize > 0 && data->data != NULL){
        struct data_t* data_temp = data_create(data->datasize);
        memcpy(data_temp->data,data->data,data->datasize);
        return data_temp;
    }

    return NULL;
}

void data_replace(struct data_t *data, int new_size, void *new_data){
    free(data->data);
    data->data = new_data;
    data->datasize = new_size;
}