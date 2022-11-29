// Ficheiro desenvolvido pelo grupo 7, cujos elementos são:
//   - André Dias    , nº 55314
//   - David Pereira , nº 56361
//   - Miguel Cut    , nº 56339

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <tree.h>
#include <tree_skel.h>
#include <entry.h>
#include <data.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <tree_skel-private.h>
#include <pthread.h>
#include <stdint.h>
#include <zookeeper/zookeeper.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

struct tree_t* tree = NULL;     //Tree data structure
int last_assigned = 1;          //ID of last assigned request
struct request_t *queue_head;   //Head of the request queue
struct op_proc op_status;       //Status of the operations

//Queue sync
pthread_mutex_t queue_lock;
pthread_cond_t queue_not_empty;

//Op_Proc sync
pthread_mutex_t op_proc_lock;

//Tree sync
pthread_mutex_t tree_lock;

//Zookeeper
struct rtree_t rtree;
typedef struct String_vector zoo_string;
static char *watcher_ctx = "ZooKeeper Data Watcher";

int verify(int op_n) {

    if (op_n <= 0) {
        return -1;
    }

    if (op_n < op_status.max_proc) {
        for (int i = 0; i < (sizeof(op_status.in_progress) / sizeof(op_status.in_progress[0])); i++) {
            if (op_status.in_progress[i] == op_n) {
                return 0;
            }
        }
    }

    if (op_n > op_status.max_proc) {
        for (int i = 0; i < (sizeof(op_status.in_progress) / sizeof(op_status.in_progress[0])); i++) {
            if (op_status.in_progress[i] == op_n) {
                return 0;
            }
        }
        return -1;
    }

    return 1;
}

int tree_skel_init(char* zHost, char* address){

    char* zHost_temp = malloc(sizeof(char) * strlen(zHost) + 1);
    strcpy(zHost_temp , zHost);

    char* address_temp = malloc(sizeof(char) * strlen(address) + 1);
    strcpy(address_temp , address);

    //Connect to zookeper
    zookeeper_connect(zHost_temp , address_temp);

    tree = tree_create();

    op_status.max_proc = 0;
    op_status.in_progress = malloc(sizeof(int));

    queue_head = NULL;

    pthread_mutex_init(&queue_lock, NULL);
    pthread_cond_init(&queue_not_empty, NULL);

    pthread_mutex_init(&op_proc_lock, NULL);

    pthread_mutex_init(&tree_lock, NULL);

    if(tree == NULL){
        return -1;
    }

    //Create threads
    pthread_t thread;

    int i = 1;

    int result = pthread_create(&thread, NULL, process_request, (void *)(intptr_t)i);
    pthread_detach(thread);

    if(result < 0){
        perror("Failed to create thread");
        return -1;
    }

    return 0;
}

int zookeeper_connect(char* host, char* sv_port){

    zhandle_t* zh = zookeeper_init(host, watcher_server, 2000, 0, NULL, 0);
    char* root = "/";

    if (zh == NULL){
        perror("Error connecting to ZooKeeper server!\n");
        return -1;
    }

    printf("Conectado ao zookeeper! \n");

    rtree.handler = zh;

    //Check if /chain already exists

    int retval = zoo_exists(zh, "/chain", 0, NULL);

    if(retval == ZNONODE){
        //Chain does not exist, create

        retval = zoo_create(zh, "/chain", NULL, -1, & ZOO_OPEN_ACL_UNSAFE, 0 , NULL, 0);

        if(retval != ZOK) {
            perror("Error creating chain node!");
            return -1;
        }

    } else {
        //Chain exists, create child
        char* node = malloc(1024);

        char* IPport = malloc(1024);

        char* IPbuffer;

        get_computer_ip(&IPbuffer);

        strcat(IPport, IPbuffer);
        strcat(IPport, ":");
        strcat(IPport, sv_port);

        int size = strlen(IPport) + 1;

        retval = zoo_create(zh, "/chain/node", IPport, size, & ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL | ZOO_SEQUENCE, node, 1024);

        if(retval != ZOK){
            perror("Error creating client node!");
            return -1;
        }

        free(IPport);

        rtree.zMyNode = node;

        zoo_string* children_list = malloc(sizeof(zoo_string));

        retval = zoo_wget_children(zh, "/chain", &child_watcher,watcher_ctx, children_list);

        if (retval != ZOK) {
            perror("Error getting child list!");
            return -1;
        }
        
        if (children_list->count >= 2){
            
             char* path = malloc(1024);

            char* next_node = children_list->data[children_list->count - 2];

            rtree.zNextNode = next_node;
            
            char* chain = "/chain/";

            strcat(path, chain);

            strcat(path, next_node);
            
            int* buffer_len = malloc(sizeof(int));
            *buffer_len = 1024;

            char* buffer = malloc(*buffer_len);

            retval = zoo_get(zh, path, 0 , buffer, buffer_len, NULL);

            if (retval != ZOK){
                perror("Error getting metadata from node!");
                return -1;
            }

            free(path);

            if(connect_to_server(buffer) != 0) {
                perror("Error connecting to chain server!");
                return -1;
            }


        } else {
            //This is last node
            rtree.zNextNode = NULL;
        }

        return 0;
    }


}

int connect_to_server(char* address){

    printf("Address: %s\n", address);

    return 0;

}

int get_computer_ip(char** buffer){
    
    char* IP;

    struct hostent* host_entry;
    char hostbuffer[256];

    if(gethostname(hostbuffer, sizeof(hostbuffer)) != 0){
        perror("Error getting hostname!");
        return -1;
    }

    host_entry = gethostbyname(hostbuffer);

    if(host_entry == NULL){
        perror("Error getting ip address!");
        return -1;
    }

    IP = inet_ntoa(*((struct in_addr*) host_entry->h_addr_list[0]));

    *buffer = IP;

    return 0;
}

void child_watcher(zhandle_t *zh, int type, int state, const char *zpath, void *watcher_ctx){
    
}

void watcher_server(zhandle_t *zh, int type, int state, const char *path, void *watcherCtx){
    
    printf("Servidor Conectado! \n");

}

void tree_skel_destroy(){
    
    if(tree == NULL){
        return;
    }

    tree_destroy(tree);
    free(queue_head);
    free(op_status.in_progress);
}

void * process_request (void *params){
    
    int thread_id = (intptr_t) params;

    while(1) {

        pthread_mutex_lock(&queue_lock);

        while(queue_head == NULL){
            pthread_cond_wait(&queue_not_empty, &queue_lock);
        }

        struct request_t* request = queue_head;
        queue_head = request->next;

        pthread_mutex_unlock(&queue_lock);

        //Process request....

        pthread_mutex_lock(&op_proc_lock);

        op_status.in_progress[thread_id] = request->op_n;

        pthread_mutex_unlock(&op_proc_lock);

        int result = -1;

        if(request->op == 1){
            //put
            result = put(request->key, request->data);
            
        } else if (request->op == 0){
            result = del(request->key);
        } else {
            //erro
        }

        pthread_mutex_lock(&op_proc_lock);

        if(request->op_n > op_status.max_proc)
            op_status.max_proc = request->op_n;

        op_status.in_progress[thread_id] = 0;

        pthread_mutex_unlock(&op_proc_lock);

        //Destroy request
        free(request);

    }

}

int queue_add(MessageT* msg){
    
    pthread_mutex_lock(&queue_lock);

    struct request_t* request = malloc(sizeof(struct request_t));
    
    request->op_n = last_assigned;

    if(msg->opcode == 50){
        //put
        request->op = 1;

        request->key = malloc(strlen(msg->entry->key)*sizeof(char) + 1);
        strcpy(request->key, msg->entry->key);
        
        request->data = data_create(msg->entry->data.len);
        memcpy(request->data->data, msg->entry->data.data, request->data->datasize);

    } else if (msg->opcode = 30) {
        //del
        request->op = 0;
        
        request->key = malloc(msg->data.len);
        memcpy(request->key, msg->data.data, msg->data.len);
        
        request->data = NULL;
    } else {
        msg->opcode = 99;
        msg->c_type = 70;
        return -1;
    }
    
    if(queue_head == NULL){
        queue_head = request;
        request->next = NULL;
    } else {
        struct request_t* rptr = queue_head;
        while(rptr->next != NULL ){
            rptr=rptr->next;
        }

        rptr->next = request;
        request->next = NULL;
    }

    pthread_cond_signal(&queue_not_empty);
    pthread_mutex_unlock(&queue_lock);

    msg->opcode++;
    msg->c_type = 60;
    msg->number = request->op_n;

    last_assigned++;

    return 0;
}

int invoke(MessageT *msg){

    if(tree == NULL) {
        perror("Tree data structure not initialized");
        return -1;
    }

    int op_code = msg->opcode;
    int result;
    
    switch (op_code)
    {
    case 10:

        return size(msg);
    
    case 20:

        return height(msg);

    case 30:
        
        //Add to queue

        return queue_add(msg);

    case 40:

        return get(msg);

    case 50:
        
        //Add to queue

        return queue_add(msg);

    case 60:

        return get_keys(msg);

    case 70:

        return get_values(msg);

    case 80:

        result = verify(msg->number);

        if (result >= 0) {
            msg->opcode = 81;
            msg->c_type = 60;
            msg->number = result;
        }
        else {
            msg->opcode = 99;
            msg->c_type = 70;
        }

        return 0;

    default:
        //Bad op
        printf("ERROR\n");
        return 0;
    }

    return -1;
}



int put(char* key, struct data_t* data) {

    pthread_mutex_lock(&tree_lock);

    printf("Putting value %s with key %s on tree\n", (char*)data->data, key);

    int result = tree_put(tree, key, data);
    data_destroy(data);
    free(key);

    pthread_mutex_unlock(&tree_lock);

    return result;
}

int del(char* key) {
    
    pthread_mutex_lock(&tree_lock);

    printf("Deleting entry with key %s on tree\n", key);

    int result = tree_del(tree, key);

    free(key);

    pthread_mutex_unlock(&tree_lock);

    return result;
}

int get(MessageT *msg) {

    char* key = malloc(msg->data.len);
    strcpy(key, msg->data.data);

    printf("Getting element from tree with key %s...\n", key);

    if(key == NULL) {
        msg->opcode = 99;
        msg->c_type = 70;
        perror("Bad key received");
        return -1;
    }

    struct data_t* result;

    result = tree_get(tree, key);

    if(result == NULL) {
        //key not found
        printf("Key not found\n");
        msg->opcode = 41;
        msg->c_type = 20;
        msg->data.len = 0;
        msg->data.data = NULL;
        free(key);
        return 0;
    } 

    //key found
    msg->opcode = 41;
    msg->c_type = 20;
    msg->data.len = result->datasize;
    msg->data.data = malloc(result->datasize);
    memcpy(msg->data.data, result->data, result->datasize);

    printf("Len: %d\n", result->datasize);
    printf("Key: %s\n", msg->data.data);

    data_destroy(result);
    free(key);
    return 0;
}

int size(MessageT* msg) {

    printf("Getting size of tree\n");

    int size = tree_size(tree);

    if (size < 0) {
        msg->opcode = 99;
        msg->c_type = 70;
        return -1;
    }

    int encoded_size = htons(size);

    msg->opcode = 11;
    msg->c_type = 60;
    msg->number = size;
    return 0;
}

int height(MessageT* msg) {

    printf("Getting height of tree\n");

    int height = tree_height(tree);

    if (height < 0) {
        msg->opcode = 99;
        msg->c_type = 70;
        return -1;
    }

    msg->opcode = 21;
    msg->c_type = 60;
    msg->number = height;
    return 0;
}

int get_keys(MessageT* msg) {
    
    printf("Getting keys from tree...\n");
    
    char** keys = tree_get_keys(tree);
    if(keys == NULL) {
        //erro
        printf("Error getting keys\n");
        msg->opcode = 99;
        msg->c_type = 70;
        return -1;
    }

    msg->opcode = 61;
    msg->c_type = 40;
    msg->n_keys = tree_size(tree);
    msg->keys = keys;

    return 0;
}

int get_values(MessageT* msg) {

    printf("Getting values from tree...\n");

    struct data_t** values = (struct data_t**)tree_get_values(tree);

    if(values == NULL) {

        //erro
        msg->opcode = 99;
        msg->c_type = 70;
        return -1;
    }

    msg->opcode = 71;
    msg->c_type = 50;
    msg->n_values = tree_size(tree);
    msg->values = malloc(sizeof(MessageT__Value*) * msg->n_values);
    
    for(int i = 0; i < msg->n_values;i++){
        
        msg->values[i] = malloc(sizeof(MessageT__Value) * msg->n_values);

        message_t__value__init(msg->values[i]);

        msg->values[i]->data.len = values[i]->datasize;
        
        msg->values[i]->data.data = values[i]->data;
    }

    return 0;
}