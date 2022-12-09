// Ficheiro desenvolvido pelo grupo 7, cujos elementos são:
//   - André Dias    , nº 55314
//   - David Pereira , nº 56361
//   - Miguel Cut    , nº 56339

#include "inet.h"
#include "sdmessage.pb-c.h"
#include "zookeeper/zookeeper.h"

/* Estrutura que guarda os dados de ligação (head e tail)
*/
struct rtree_t {
    struct sockaddr_in* rtree_headAddr;
    struct sockaddr_in* rtree_tailAddr;
    int rtree_head;
    int rtree_tail;
};

/* Função que é chamada quando existem alterações
na conexão entre o cliente e o ZooKeeper
*/
void connection_watcher(zhandle_t *zzh, int type, int state, const char *path, void* context);

/* Função que é chamada quando existe uma alteração num dado node
*/
void child_watcher(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx);

/* Função que constrói e devolve a rtree necessária
para efetuar as ligações aos servidores head e tail
*/
struct rtree_t* set_rtree(zoo_string* children_list);

/* Função que ordena uma dada lista
*/
void sort_list(zoo_string** children_list);

/* Função que, dado o endereço de um servidor ZooKeeper,
estabelece conexão com o mesmo
*/
int zookeeper_connect(const char* address_port);