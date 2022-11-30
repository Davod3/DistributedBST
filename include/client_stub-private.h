// Ficheiro desenvolvido pelo grupo 7, cujos elementos são:
//   - André Dias    , nº 55314
//   - David Pereira , nº 56361
//   - Miguel Cut    , nº 56339

#include "inet.h"
#include "sdmessage.pb-c.h"
#include "zookeeper/zookeeper.h"


struct rtree_t {
    //int sockfd;
    //struct sockaddr_in* serverAddr;
    struct sockaddr_in* rtree_headAddr;
    struct sockaddr_in* rtree_tailAddr;
    int rtree_head;
    int rtree_tail;
};

void watcher_client(zhandle_t *zzh, int type, int state, const char *path, void *watcherCtx);

void connection_watcher(zhandle_t *zzh, int type, int state, const char *path, void* context);

void get_chain_children(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx);