// Ficheiro desenvolvido pelo grupo 7, cujos elementos são:
//   - André Dias    , nº 55314
//   - David Pereira , nº 56361
//   - Miguel Cut    , nº 56339

#include "inet.h"
#include "sdmessage.pb-c.h"
#include "zookeeper/zookeeper.h"


struct rtree_t {
    int sockfd;
    struct sockaddr_in* serverAddr;
};

void watcher_client(zhandle_t *zzh, int type, int state, const char *path, void *watcherCtx);