// Ficheiro desenvolvido pelo grupo 7, cujos elementos são:
//   - André Dias    , nº 55314
//   - David Pereira , nº 56361
//   - Miguel Cut    , nº 56339

#include <inet.h>
#include <sdmessage.pb-c.h>
#include <tree_skel.h>
#include <network_server.h>
#include <message-private.h>
#include <poll.h>
#include <network_server-private.h>

int socketfd;

int network_server_init(short port){

    struct sockaddr_in server;

    //Create socket
    if((socketfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("Erro ao criar socket!");
        return -1;
    }

    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0){
        perror("setsockopt(SO_REUSEADDR)");
        return -1;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr= htonl(INADDR_ANY);
    
    //Bind to port
    if(bind(socketfd, (struct sockaddr *) &server, sizeof(server)) < 0){
        perror("Erro ao dar bind!");
        close(socketfd);
        return -1;
    }

    return socketfd;
}

void remove_fd(struct pollfd** desc_set, int fd, int size) {


    struct pollfd* set = (*desc_set);
    struct pollfd* result = malloc((size-1) * sizeof(struct pollfd));
    int deleted = 0;

    for(int i = 0; i < size; i++) {

        if(set[i].fd == fd){
            deleted = 1;
            continue;
        }


        if(deleted == 1) {
            result[i-1] = set[i];
        } else {
            result[i] = set[i];
        }
    }

    free(*desc_set);
    *desc_set = result;

}

int network_main_loop(int listening_socket){

    struct pollfd listeningfd;
    listeningfd.fd = listening_socket;
    listeningfd.events = POLLIN;
    listeningfd.revents = 0;

    struct pollfd* desc_set = malloc(sizeof(struct pollfd));
    desc_set[0] = listeningfd;
    int nfds = 1;
    
    int connection_socket;
    struct sockaddr_in* client = NULL;
    socklen_t* size_client = NULL;
    MessageT *message;

    //Begin listening
    if(listen(listening_socket, 0) < 0){
        perror("Erro ao dar listen!");
        close(listening_socket);
        return -1;
    }

    //Wait for connections
    printf("Awaiting connection....\n");

    while(poll(desc_set, nfds, -1) >= 0) {

        if(desc_set[0].revents & POLLIN) {
            //Dados na listening socket
            connection_socket = accept(desc_set[0].fd, (struct sockaddr*) client, size_client);
            
            if(connection_socket < 0) {
                perror("Error accepting connection...");
                close(desc_set[0].fd);
                free(desc_set);
                return -1;
            }

                        
            nfds++;
            desc_set = realloc(desc_set, nfds*sizeof(struct pollfd));
            desc_set[nfds-1].fd = connection_socket;
            desc_set[nfds-1].events = POLLIN;
            desc_set[nfds-1].revents = 0;

        }


        for(int i = 1; i < nfds; i++){

            if(desc_set[i].revents & POLLIN){
                //Dados para leitura
                message = network_receive(desc_set[i].fd);

                if(message == NULL) {
                    close(desc_set[i].fd);
                    remove_fd(&desc_set, desc_set[i].fd, nfds);
                    nfds--;
                    continue;
                }

                if(invoke(message) < 0){
                    printf("Erro no invoke\n");
                    close(desc_set[i].fd);
                    remove_fd(&desc_set, desc_set[i].fd, nfds);
                    nfds--;
                    continue;
                }

                if(network_send(desc_set[i].fd, message) < 0){
                    printf("Erro ao enviar resposta!");
                    close(desc_set[i].fd);
                    remove_fd(&desc_set, desc_set[i].fd, nfds);
                    nfds--;
                    continue;
                }

                message_t__free_unpacked(message, NULL);
            }

            if(desc_set[i].revents & POLLERR || desc_set[i].revents & POLLHUP || desc_set[i].revents & POLLNVAL){
                printf("Invalid file descriptor");
                close(desc_set[i].fd);
                remove_fd(&desc_set, desc_set[i].fd, nfds);
                nfds--;
                continue;
            }
        }
    }
    free(desc_set);
    return 0;
}



MessageT *network_receive(int client_socket){

    int len;
    MessageT *message = NULL;
    int *size_buf = malloc(sizeof(int));

    size_buf[0] = 0;

    int nbytes;

    //read size
    if(read_all(client_socket, size_buf, sizeof(int)) < 0){
        free(size_buf);
        return NULL;
    }

    len = ntohs(*size_buf);

    //Read bytes

    uint8_t *buf = malloc(len);

    if(read_all(client_socket, buf, len) < 0){
        free(buf);
        return NULL;
    }

    message = message_t__unpack(NULL, len, buf);

    if(message == NULL){
        printf("Erro na de-serialiação");
        free(buf);
        return NULL;
    }

    free(size_buf);
    free(buf);

    return message;
}

int network_send(int client_socket, MessageT *msg){
    
    int nbytes = 0;
    uint8_t* buf = NULL;

    int len = message_t__get_packed_size(msg);
    buf = malloc(len);

    message_t__pack(msg, buf);

    //Send size
    int len_buf = htons(len);

    if(write_all(client_socket, &len_buf, sizeof(int)) != sizeof(int)){
        free(buf);
        return -1;
    }

    //Send data

    if((write_all(client_socket, buf, len)) != len){
        free(buf);
        return -1;
    }

    free(buf);

    return 0;
}

int network_server_close() {

    if(close(socketfd) < 0) {
        perror("Error closing socket");
        return -1;
    }

    return 0;
}