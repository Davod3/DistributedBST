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
#include <sys/types.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <message-private.h>

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

void send_receive(MessageT *msg){

    printf("Talking with server....\n");

    if(rtree.zNextNode == NULL){
        return;
    }

    int socketfd = rtree.nextSocket;

    //Serialize message:
    int length;
    uint8_t* buffer;

    length = message_t__get_packed_size(msg);
    buffer = malloc(length);

    if(buffer == NULL){
        perror("Buffer error");
        return;
    }

    message_t__pack(msg, buffer);

    //Send message to server:

    //Send size
    int length_buf = htons(length);

    if(write_all(socketfd, &length_buf, sizeof(int)) < 0){
        return;
    }

    //Send data
    if(write_all(socketfd, buffer, length) != length){
        return;
    }

    free(buffer);

    // Receive message from server

    //Receive size
    int* length_temp = malloc(sizeof(int));

    if(read_all(socketfd, length_temp, sizeof(int)) < 0){
        return;
    }

    length = ntohs(*length_temp);

    free(length_temp);

    // Receive message
    
    uint8_t* buffer_rcv = malloc(length);

    if(buffer_rcv == NULL){
        perror("Buffer error");
        return;
    }

    if((read_all(socketfd, buffer_rcv,length)) != length){
        return;
    }

    MessageT* received = NULL;

    received = message_t__unpack(NULL,length,buffer_rcv);
    if (received == NULL) {
        perror("Error unpacking message\n");
        return;
    }

    free(buffer_rcv);

    message_t__free_unpacked(received, NULL);
}

void propagate_request(struct request_t* request){

    printf("Propagating request to servers.....\n");

    if(rtree.zNextNode == NULL){
        return;
    }

    MessageT msg;
    message_t__init(&msg);

    if(request->op == 0){
        //delete

        msg.opcode = 30;
        msg.c_type = 10;
        msg.data.len = strlen(request->key) + 1;
        msg.data.data = malloc(msg.data.len * sizeof(char));
        memcpy(msg.data.data, request->key, msg.data.len);

        printf("Created del message! \n");

    } else {
        //put

        msg.opcode = 50;
        msg.c_type = 30;

        MessageT__Entry temp_entry;
        message_t__entry__init(&temp_entry);

        //copy key
        temp_entry.key = malloc(sizeof(char) * strlen(request->key) + 1);

        strcpy(temp_entry.key, request->key);

        //copy data
        ProtobufCBinaryData temp_data;

        temp_data.len = request->data->datasize;

        temp_data.data = malloc(request->data->datasize);

        memcpy(temp_data.data, request->data->data, request->data->datasize);

        temp_entry.data = temp_data;

        msg.entry = &temp_entry;


        printf("Created put message with key %s - value %s! \n", msg.entry->key, msg.entry->data.data);

    }

    send_receive(&msg);
}

void sort_list(zoo_string** children_list){
    int j;
    int i;
    char* temp;


    for(i=0; i < (*children_list)->count ;i++){
      
      for(j=i+1; j< (*children_list)->count ;j++){
         
         if(strcmp((*children_list)->data[i], (*children_list)->data[j] ) > 0){
            
            temp = malloc(strlen((*children_list)->data[i]) + 1);

            strcpy(temp, (*children_list)->data[i] );
            strcpy((*children_list)->data[i], (*children_list)->data[j] );
            strcpy((*children_list)->data[j],temp);

            free(temp);

         }
      }

    }
}

int get_computer_ip(char** buffer){

    FILE *pf;

    char command[1024];
    char data[512];

    //Set command
    sprintf(command, "ip route get 1.1.1.1 | grep -oP 'src \\K\\S+'");

    pf = popen(command , "r");

    fgets(data, 512 , pf);
    int len = strlen(data);
    data[len-1] = '\0';

    *buffer = data;

    return 0;
}

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

int zookeeper_connect(char* host, char* sv_port){

    zoo_set_debug_level((ZooLogLevel)0);

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

    }

    //Chain exists, create child
    char* node = malloc(1024);

    char* IPport = malloc(1024);

    char* IPbuffer = NULL;

    get_computer_ip(&IPbuffer);

    printf("Gets here!\n");

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

    child_watcher(zh, 0, 0, NULL, watcher_ctx);

    return 0;


}

int tree_skel_init(char* zHost, char* address){

    char* zHost_temp = malloc(sizeof(char) * strlen(zHost) + 1);
    strcpy(zHost_temp , zHost);

    char* address_temp = malloc(sizeof(char) * strlen(address) + 1);
    strcpy(address_temp , address);

    //Connect to zookeper
    zookeeper_connect(zHost_temp , address_temp);

    free(address_temp);
    free(zHost_temp);

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

int connect_to_server(char* address){

    printf("Address: %s\n", address);
    
    //Prevent race conditions
    sleep(2);

    char* address_temp = malloc(sizeof(char) * strlen(address) + 1);
    strcpy(address_temp, address);
    
    char* token = strtok(address_temp, ":");

    char** address_split = malloc(2*sizeof(char*));

    int pointer = 0;

    while (token != NULL) {
            address_split[pointer] = token;

            token = strtok(NULL, ":");
            pointer++;
    }

    if(address_split[0] == NULL || address_split[1] == NULL) {
        printf("Failed to connect to tree\n");
        free(address_temp);
        free(address_split);
        return -1;
    }

    printf("Connecting to ip: %s, port: %s\n", address_split[0], address_split[1]);

    struct sockaddr_in server;

    // Preenche estrutura server para estabelecer conexão
    server.sin_family = AF_INET;
    server.sin_port = htons(atoi(address_split[1])); //adress[1] é TCP
    server.sin_addr.s_addr = inet_addr(address_split[0]);

    int socketfd;

    // Cria socket TCP
    if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Erro ao criar socket TCP");
        return -1;
    }


    // Estabelece conexão com o servidor definido em server
    if (connect(socketfd,(struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Erro ao conectar-se ao servidor");
        close(socketfd);
        return -1;
    }

    rtree.nextSocket = socketfd;

    return 0;

}

void child_watcher(zhandle_t *zh, int type, int state, const char *zpath, void *watcher_ctx){
    
    char* chain = "/chain";

    zoo_string* children_list = malloc(sizeof(zoo_string));

    int retval = zoo_wget_children(zh, chain, &child_watcher,watcher_ctx, children_list);


    if (retval != ZOK) {
        free(children_list);
        perror("Error getting child list!");
        return;
    }
        
    if (children_list->count >= 2){

         sort_list(&children_list);

         int counter = 0;

         char* next = NULL;

         int keepGoing = 1;

         while(counter < children_list->count && keepGoing == 1) {

            next = malloc(1024);
            *next = '\0';
            
            strcat(next, chain);
            strcat(next, "/");
            strcat(next, children_list->data[counter]);

            if(strcmp(rtree.zMyNode, next) < 0){
                keepGoing = 0;
                break;
            } 

            counter++;
            free(next);
            next = NULL;

         }

         rtree.zNextNode = next;

         if(next != NULL) {

            int* buffer_len = malloc(sizeof(int));
            *buffer_len = 1024;

            char* buffer = malloc(*buffer_len);

            retval = zoo_get(zh, next, 0 , buffer, buffer_len, NULL);

            if (retval != ZOK){
                free(children_list);
                perror("Error getting metadata from node!");
                return;
            }

            if(connect_to_server(buffer) != 0) {
                free(children_list);
                perror("Error connecting to chain server!");
                return;
            }
        }

    } else {
        //This is last node
        rtree.zNextNode = NULL;
    }
    free(children_list);
    return;
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
            
            if(result == 0){
                //propagate
                propagate_request(request);
            }

            data_destroy(request->data);
            free(request->key);

        } else if (request->op == 0){
            result = del(request->key);

            if(result == 0){
                //propagate
                propagate_request(request);
            }

            free(request->key);

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
    //data_destroy(data);
    //free(key);

    pthread_mutex_unlock(&tree_lock);

    return result;
}

int del(char* key) {
    
    pthread_mutex_lock(&tree_lock);

    printf("Deleting entry with key %s on tree\n", key);

    int result = tree_del(tree, key);

    //free(key);

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