// Ficheiro desenvolvido pelo grupo 7, cujos elementos são:
//   - André Dias    , nº 55314
//   - David Pereira , nº 56361
//   - Miguel Cut    , nº 56339

#include "client_stub.h"
#include "sdmessage.pb-c.h"
#include "inet.h"
#include "client_stub-private.h"
#include "network_client.h"
#include "message-private.h"

int network_connect(struct rtree_t *rtree) {

    //int socketfd;
    int rtree_hd_sock;
    int rtree_tl_sock;

    // Cria socket TCP para a head
    if ((rtree_hd_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Erro ao criar socket TCP para a head");
        return -1;
    }

    // Cria socket TCP para a tail
    if ((rtree_tl_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Erro ao criar socket TCP para a tail");
        return -1;
    }

    // struct sockaddr_in server;
    // server.sin_family = rtree->serverAddr->sin_family;
    // server.sin_port = rtree->serverAddr->sin_port;
    // server.sin_addr.s_addr = rtree->serverAddr->sin_addr.s_addr;

    struct sockaddr_in server_head;
    server_head.sin_family = rtree->rtree_headAddr->sin_family;
    server_head.sin_port = rtree->rtree_headAddr->sin_port;
    server_head.sin_addr.s_addr = rtree->rtree_headAddr->sin_addr.s_addr;

    struct sockaddr_in server_tail;
    server_tail.sin_family = rtree->rtree_tailAddr->sin_family;
    server_tail.sin_port = rtree->rtree_tailAddr->sin_port;
    server_tail.sin_addr.s_addr = rtree->rtree_tailAddr->sin_addr.s_addr;


    // Estabelece conexão com o servidor definido em server para a head
    if (connect(rtree_hd_sock,(struct sockaddr *)&server_head, sizeof(server_head)) < 0) {
        perror("Erro ao conectar-se ao servidor hed");
        close(rtree_hd_sock);
        return -1;
    }

    // Estabelece conexão com o servidor definido em server para a tail
    if (connect(rtree_tl_sock,(struct sockaddr *)&server_tail, sizeof(server_tail)) < 0) {
        perror("Erro ao conectar-se ao servidor tail");
        close(rtree_tl_sock);
        return -1;
    }

    rtree->rtree_head = rtree_hd_sock;
    rtree->rtree_tail = rtree_tl_sock;


    return 0;
}

MessageT *network_send_receive(struct rtree_t * rtree, MessageT *msg) {
    
    printf("Talking with server....\n");

    int socketfd;
    //Check if it's write or read
    if(msg->opcode == 50 || msg->opcode == 30)
        socketfd = rtree->rtree_head;
    else    
        socketfd = rtree->rtree_tail;
    //Get socket fd:

    //Serialize message:
    int length;
    uint8_t* buffer;

    length = message_t__get_packed_size(msg);
    buffer = malloc(length);

    if(buffer == NULL){
        perror("Buffer error");
        return NULL;
    }

    message_t__pack(msg, buffer);

    //Send message to server:

    //Send size
    int length_buf = htons(length);

    if(write_all(socketfd, &length_buf, sizeof(int)) < 0){
        return NULL;
    }

    //Send data
    if(write_all(socketfd, buffer, length) != length){
        return NULL;
    }

    free(buffer);

    // Receive message from server

    //Receive size
    int* length_temp = malloc(sizeof(int));

    if(read_all(socketfd, length_temp, sizeof(int)) < 0){
        return NULL;
    }

    length = ntohs(*length_temp);

    free(length_temp);

    // Receive message
    
    uint8_t* buffer_rcv = malloc(length);

    if(buffer_rcv == NULL){
        perror("Buffer error");
        return NULL;
    }

    if((read_all(socketfd, buffer_rcv,length)) != length){
        return NULL;
    }

    MessageT* received = NULL;

    received = message_t__unpack(NULL,length,buffer_rcv);
    if (received == NULL) {
        perror("Error unpacking message\n");
        return NULL;
    }

    free(buffer_rcv);

    return received;                                        
}

int network_close(struct rtree_t * rtree) {
    if(close(rtree->rtree_head) == -1)
        return -1;
    else
        return close(rtree->rtree_tail);
}