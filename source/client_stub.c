// Ficheiro desenvolvido pelo grupo 7, cujos elementos são:
//   - André Dias    , nº 55314
//   - David Pereira , nº 56361
//   - Miguel Cut    , nº 56339

#include "data.h"
#include "entry.h"
#include <stdio.h>
#include <string.h>
#include <inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <network_client.h>
#include <client_stub-private.h>
#include <client_stub.h>
#include <sdmessage.pb-c.h>
#include "zookeeper/zookeeper.h"

typedef struct String_vector zoo_string; 

static zhandle_t *zh;
static int is_connected;
//zoo_string* children_list;
static char *chain_path = "/chain";
static char *watcher_ctx = "ZooKeeper Data Watcher";

struct rtree_t* rtree_stub;

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



int zookeeper_connect(const char* address_port){

    zoo_set_debug_level((ZooLogLevel)0);

    zh = zookeeper_init(address_port, connection_watcher, 2000, 0, NULL, 0); 
	if (zh == NULL)	{
		perror("Error connecting to ZooKeeper server!\n");
        zookeeper_close(zh);
        return -1;
	}

    printf("Conectado ao zookeeper!\n");
    return 0;
}



struct rtree_t *rtree_connect(const char *address_port) {
    if(zookeeper_connect(address_port) == -1){
        printf("O ZooKeeper não está ligado!\n");
        zookeeper_close(zh);
        return NULL;
    }

    zoo_string* children_list =	(zoo_string *) malloc(sizeof(zoo_string));

    if (ZOK != zoo_wget_children(zh, chain_path, child_watcher, watcher_ctx, children_list)) {
        printf("Error setting watch at %s!\n", chain_path);
        zookeeper_close(zh);
        free(children_list);
        return NULL;
    }

    sort_list(&children_list);

    if (children_list == NULL || children_list->count == 0) {
        printf("O ZooKeeper não tem nodes!\n");
        free(children_list);
        zookeeper_close(zh);
        return NULL;
    }

    rtree_stub = set_rtree(children_list);

    if (network_connect(rtree_stub) < 0) {
        perror("Failed to connect to tree");
        free(children_list);
        zookeeper_close(zh);
        return NULL;
    }

    free(children_list);
    return rtree_stub;
}

void connection_watcher(zhandle_t *zzh, int type, int state, const char *path, void* context) {
	if (type == ZOO_SESSION_EVENT) {
		if (state == ZOO_CONNECTED_STATE) {
			is_connected = 1; 
		} else {
			is_connected = 0;
		}
	} 
}


struct rtree_t* set_rtree(zoo_string* children_list) {
    char* path_hd = malloc(1024);
    char* path_tl = malloc(1024);

    for (int i = 0; i < 1024; i++) {
        path_hd[0] = '\0';
        path_tl[0] = '\0';
    }
    
    char* chain = "/chain/";

    strcat(path_hd, chain);
    strcat(path_tl, chain);

    if(children_list == NULL || children_list->count == 0) {
        printf("O ZooKeeper não tem servidores disponíveis!\n");
        zookeeper_close(zh);
        exit(-1);
    }

    char* temp1 = children_list->data[0];
    char* temp2 = children_list->data[children_list->count-1];

    strcat(path_hd, temp1);
    strcat(path_tl, temp2);

    int* addr_temp_hd_len = malloc(sizeof(int));
    *addr_temp_hd_len = 1024;
    int* addr_temp_tl_len = malloc(sizeof(int));
    *addr_temp_tl_len = 1024;
    char* address_temp_hd = malloc(*addr_temp_hd_len);
    char* address_temp_tl = malloc(*addr_temp_tl_len);

    int get_hd_val = zoo_get(zh, path_hd, 0, address_temp_hd, addr_temp_hd_len, NULL);
    if(get_hd_val == -1){
        perror("Error setting watch for head");
        free(path_hd);
        free(path_tl);
        free(addr_temp_hd_len);
        free(addr_temp_tl_len);
        free(address_temp_hd);
        free(address_temp_tl);
        zookeeper_close(zh);
        return NULL;
    }

    int get_tl_val = zoo_get(zh, path_tl, 0, address_temp_tl, addr_temp_tl_len, NULL);
    if(get_tl_val == -1){
        perror("Error setting watch for tail");
        free(path_hd);
        free(path_tl);
        free(addr_temp_hd_len);
        free(addr_temp_tl_len);
        free(address_temp_hd);
        free(address_temp_tl);
        zookeeper_close(zh);
        return NULL;
    }

    char* ip_hd = strtok(address_temp_hd, ":");
    char* port_hd = strtok(NULL, ":");

    char* ip_tl = strtok(address_temp_tl, ":");
    char* port_tl = strtok(NULL, ":");

    if(ip_hd == NULL || port_hd == NULL || ip_tl == NULL || port_tl == NULL) {
        printf("Failed to connect to tree\n");
        free(path_hd);
        free(path_tl);
        free(addr_temp_hd_len);
        free(addr_temp_tl_len);
        free(address_temp_hd);
        free(address_temp_tl);
        zookeeper_close(zh);
        return NULL;
    }

    printf("Connecting to head ip: %s, port: %s\n", ip_hd, port_hd);
    printf("Connecting to tail ip: %s, port: %s\n", ip_tl, port_tl);

    struct sockaddr_in* server_hd = malloc(sizeof(struct sockaddr_in));
    struct sockaddr_in* server_tl = malloc(sizeof(struct sockaddr_in));

    // Preenche estrutura server_hd para estabelecer conexão
    server_hd->sin_family = AF_INET;
    server_hd->sin_port = htons(atoi(port_hd)); //adress[1] é TCP
    server_hd->sin_addr.s_addr = inet_addr(ip_hd);

    server_tl->sin_family = AF_INET;
    server_tl->sin_port = htons(atoi(port_tl)); //adress[1] é TCP
    server_tl->sin_addr.s_addr = inet_addr(ip_tl);

    struct rtree_t* rtree = malloc(sizeof(struct rtree_t));
    rtree->rtree_headAddr = server_hd;
    rtree->rtree_tailAddr = server_tl;

    free(path_hd);
    free(path_tl);
    free(addr_temp_hd_len);
    free(addr_temp_tl_len);
    free(address_temp_hd);
    free(address_temp_tl);

    return rtree;
}



void child_watcher(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx){

    zoo_string* children_list =	(zoo_string *) malloc(sizeof(zoo_string));

    if (state == ZOO_CONNECTED_STATE) {
        if (type == ZOO_CHILD_EVENT) {
            /* Get the updated children and reset the watch */ 
            if (ZOK != zoo_wget_children(zh, chain_path, child_watcher, watcher_ctx, children_list)) {
                printf("Error setting watch at %s!\n", chain_path);
            }

            sort_list(&children_list);

            if(rtree_stub != NULL) {

                network_close(rtree_stub);
                free(rtree_stub->rtree_headAddr);
                free(rtree_stub->rtree_tailAddr);
                free(rtree_stub);

            }

            rtree_stub = set_rtree(children_list);

            if (network_connect(rtree_stub) < 0) {
                perror("Failed to connect to tree");
                free(children_list);
                zookeeper_close(zh);
                return;
            }
        } 
    }

    free(children_list); //Possible problem
}



int rtree_disconnect(struct rtree_t *rtree) {
    
    printf("This is happening!");

    if(network_close(rtree) < 0){
        perror("Error closing socket");
        zookeeper_close(zh);
        free(rtree->rtree_headAddr);
        free(rtree->rtree_tailAddr);
        free(rtree);

        return -1;
    }
    
    zookeeper_close(zh);
    free(rtree->rtree_headAddr);
    free(rtree->rtree_tailAddr);
    free(rtree);
    return 0;
}



int rtree_put(struct rtree_t *rtree, struct entry_t *entry) {
    
    printf("Putting element (%s, %s) on tree....\n", entry->key, (char*)entry->value->data);
    
    MessageT send;

    message_t__init(&send);

    send.opcode = 50;
    send.c_type = 30;

    MessageT__Entry temp_entry;
    message_t__entry__init(&temp_entry);

    //copy key
    temp_entry.key = malloc(sizeof(char) * strlen(entry->key) + 1);
    strcpy(temp_entry.key, entry->key);

    //copy data
    ProtobufCBinaryData temp_data;
    temp_data.len = entry->value->datasize;
    temp_data.data = malloc(entry->value->datasize);
    memcpy(temp_data.data, entry->value->data, entry->value->datasize);

    temp_entry.data = temp_data;

    send.entry = &temp_entry;

    MessageT* received = network_send_receive(rtree, &send);

    if(received == NULL) {
        perror("Error receiving answer: PUT");
        free(temp_entry.key);
        free(temp_data.data);
        message_t__free_unpacked(received, NULL);
        return -1;
    }
    else if (received->opcode == 99) {
        message_t__free_unpacked(received, NULL);
        free(temp_entry.key);
        free(temp_data.data);
        return -1;
    }

    int result = received->number;
    free(temp_entry.key);
    free(temp_data.data);
    message_t__free_unpacked(received, NULL);

    //After put operation, verify completion
    sleep(2);

    while(rtree_verify(rtree, result) != 1){
        //Operation has not reached tail server, redo
        result = rtree_put(rtree, entry);

        sleep(2);
    }

    //Operation completed
    return result;
}



struct data_t *rtree_get(struct rtree_t *rtree, char *key) {

    printf("Getting elem with key: %s\n", key);

    MessageT send;

    message_t__init(&send);

    send.opcode = 40;
    send.c_type = 10;

    //set data
    send.data.len = sizeof(char) * strlen(key) + 1;
    send.data.data = key;

    MessageT* received = network_send_receive(rtree, &send);

    if(received == NULL) {
        perror("Error receiving answer: GET");
        free(received);
        return NULL;
    }
    else if (received->opcode == 99) {
        message_t__free_unpacked(received, NULL);
        return NULL;
    }

    struct data_t* result = data_create2(received->data.len, received->data.data);
    free(received);
    return result;
}



int rtree_del(struct rtree_t *rtree, char *key) {

    printf("Deleting element with key %s\n", key);

    MessageT send;

    message_t__init(&send);

    send.opcode = 30;
    send.c_type = 10;

    //set data
    send.data.len = sizeof(char) * strlen(key) + 1;
    send.data.data = key;

    MessageT* received = network_send_receive(rtree, &send);

    if(received == NULL) {
        perror("Error receiving answer: DELETE");
        return -1;
    }
    else if (received->opcode == 99) {
        message_t__free_unpacked(received, NULL);
        return -1;
    }

    int result = received->number;
    message_t__free_unpacked(received, NULL);


    //After put operation, verify completion
    sleep(2);

    while(rtree_verify(rtree, result) != 1){
        //Operation has not reached tail server, redo
        result = rtree_del(rtree, key);

        sleep(2);
    }

    //Operation completed
    return result;
}



int rtree_size(struct rtree_t *rtree) {
    printf("Checking tree size....\n");

    MessageT send;

    message_t__init(&send);

    send.opcode = 10;
    send.c_type = 70;

    MessageT* received = network_send_receive(rtree, &send);

    if(received == NULL) {
        perror("Error receiving answer: SIZE");
        return -1;
    }
    else if (received->opcode == 99) {
        message_t__free_unpacked(received, NULL);
        return -1;
    }

    int value = received->number;
    message_t__free_unpacked(received, NULL);
    return value;
}



int rtree_height(struct rtree_t *rtree) {
    
    printf("Checking tree height....\n");

    MessageT send;

    message_t__init(&send);

    send.opcode = 20;
    send.c_type = 70;

    MessageT* received = network_send_receive(rtree, &send);

    if(received == NULL) {
        perror("Error receiving answer: HEIGHT");
        return -1;
    }
    else if (received->opcode == 99) {
        message_t__free_unpacked(received, NULL);
        return -1;
    }

    int value = received->number;
    message_t__free_unpacked(received, NULL);
    return value;
}



char **rtree_get_keys(struct rtree_t *rtree) {

    MessageT send;

    message_t__init(&send);

    send.opcode = 60;
    send.c_type = 70;

    MessageT* received = network_send_receive(rtree, &send);

    if(received == NULL) {
        perror("Error receiving answer: GET_KEYS");
        return NULL;
    }
    else if (received->opcode == 99) {
        message_t__free_unpacked(received, NULL);
        return NULL;
    }

    char** result = malloc(sizeof(char*) * (received->n_keys + 1));

    int i;

    for(i = 0; i < received->n_keys; i++){
        
        result[i] = malloc(strlen(received->keys[i])*sizeof(char) + 1);
        strcpy(result[i], received->keys[i]);

    }

    result[i] = NULL;

    message_t__free_unpacked(received, NULL);

    return result;
}



void **rtree_get_values(struct rtree_t *rtree) {

    MessageT send;

    message_t__init(&send);

    send.opcode = 70;
    send.c_type = 70;

    MessageT* received = network_send_receive(rtree, &send);

    if(received == NULL) {
        perror("Error receiving answer: GET_KEYS");
        return NULL;
    }
    else if (received->opcode == 99) {
        message_t__free_unpacked(received, NULL);
        return NULL;
    }

    void** values = malloc(sizeof(void*) * (received->n_values + 1));
    
    int i;

    for(i = 0; i < received->n_values; i++) {
        values[i] = malloc(received->values[i]->data.len);
        memcpy(values[i], received->values[i]->data.data, received->values[i]->data.len);
    }

    values[i] = NULL;

    message_t__free_unpacked(received, NULL);
    return values;
}



int rtree_verify(struct rtree_t *rtree, int op_n){
    
    MessageT send;

    message_t__init(&send);

    send.opcode = 80;
    send.c_type = 60;

    send.number = op_n;

    MessageT* received = network_send_receive(rtree, &send);

    if(received == NULL) {
        perror("Error receiving answer: VERIFY");
        return -1;
    }
    else if (received->opcode == 99) {
        message_t__free_unpacked(received, NULL);
        return -1;
    }

    int result = received->number;
    message_t__free_unpacked(received, NULL);
    return result;
}