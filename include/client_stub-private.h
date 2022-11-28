// Ficheiro desenvolvido pelo grupo 7, cujos elementos são:
//   - André Dias    , nº 55314
//   - David Pereira , nº 56361
//   - Miguel Cut    , nº 56339

#include "inet.h"
#include "sdmessage.pb-c.h"

struct rtree_t {
    int sockfd;
    struct sockaddr_in* serverAddr;
};