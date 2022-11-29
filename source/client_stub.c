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
zoo_string* children_list;
static char *chain_path = "/chain";
static char *watcher_ctx = "ZooKeeper Data Watcher";


struct rtree_t *rtree_connect(const char *address_port) {
    
    char* address_temp = malloc(sizeof(char) * strlen(address_port) + 1);
    strcpy(address_temp, address_port);
    
    char* token = strtok(address_temp, ":");

    char** address = malloc(2*sizeof(char*));

    int pointer = 0;

    while (token != NULL) {
            address[pointer] = token;

            token = strtok(NULL, ":");
            pointer++;
    }

    if(address[0] == NULL || address[1] == NULL) {
        printf("Failed to connect to tree\n");
        free(address_temp);
        free(address);
        return NULL;
    }

    printf("Connecting to ip: %s, port: %s\n", address[0], address[1]);

    struct sockaddr_in* server = malloc(sizeof(struct sockaddr_in));

    // Preenche estrutura server para estabelecer conexão
    server->sin_family = AF_INET;
    server->sin_port = htons(atoi(address[1])); //adress[1] é TCP
    server->sin_addr.s_addr = inet_addr(address[0]);

    struct rtree_t* rtree = malloc(sizeof(struct rtree_t));
    rtree->serverAddr = server;

    if (network_connect(rtree) < 0) {
        perror("Failed to connect to tree");
        free(address_temp);
        free(rtree);
        free(server);
        free(address);
        return NULL;
    }

    //////////////---ZOOKEEPER --////////////////////
    children_list =	(zoo_string *) malloc(sizeof(zoo_string));

    zookeeper_connect(address_port);
    if (ZOK != zoo_wget_children(zh, chain_path, get_chain_children, watcher_ctx, children_list)) {
        fprintf(stderr, "Error setting watch at %s!\n", chain_path);
    }

    printf("%d\n", children_list->count);
    for (int i = 0; i < children_list->count; i++)  {
        fprintf("\n(%d): %s", i+1, children_list->data[i]);
    }

    ///////////////---ZOOKEEPER --//////////////////////

    free(address_temp);
    free(address);

    return rtree;
}

///////////--ZOOKEEPER--//////////////////////

void connection_watcher(zhandle_t *zzh, int type, int state, const char *path, void* context) {
	if (type == ZOO_SESSION_EVENT) {
		if (state == ZOO_CONNECTED_STATE) {
			is_connected = 1; 
		} else {
			is_connected = 0; 
		}
	} 
}

void zookeeper_connect(char* address_port){
    zh = zookeeper_init(address_port, connection_watcher,	2000, 0, NULL, 0); 
	if (zh == NULL)	{
		fprintf(stderr, "Error connecting to ZooKeeper server!\n");
	    exit(EXIT_FAILURE);
	}

    printf("Conectado ao zookeeper!");
}



void get_chain_children(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx){
    printf("GETS HERE GETCHAINCHILDREN");
    children_list =	(zoo_string *) malloc(sizeof(zoo_string));
    if (state == ZOO_CONNECTED_STATE)	 {
            if (type == ZOO_CHILD_EVENT) {
            /* Get the updated children and reset the watch */ 
                if (ZOK != zoo_wget_children(zh, chain_path, get_chain_children, watcher_ctx, children_list)) {
                    fprintf(stderr, "Error setting watch at %s!\n", chain_path);
                }
                fprintf(stderr, "\n=== znode listing === [ %s ]", chain_path); 
                for (int i = 0; i < children_list->count; i++)  {
                    fprintf(stderr, "\n(%d): %s", i+1, children_list->data[i]);
                }
                fprintf(stderr, "\n=== done ===\n");
            } 
        }
    free(children_list);
}
///////////--ZOOKEEPER--//////////////////////

int rtree_disconnect(struct rtree_t *rtree) {
    
    if(network_close(rtree) < 0){
        perror("Error closing socket");
        return -1;
    }

    free(rtree->serverAddr);
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
        return -1;
    }
    else if (received->opcode == 99) {
        message_t__free_unpacked(received, NULL);
        return -1;
    }

    int result = received->number;
    free(temp_entry.key);
    free(temp_data.data);
    message_t__free_unpacked(received, NULL);
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
    printf("Verifying tree\n");
    return result;
        
}