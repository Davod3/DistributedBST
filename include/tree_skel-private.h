// Ficheiro desenvolvido pelo grupo 7, cujos elementos são:
//   - André Dias    , nº 55314
//   - David Pereira , nº 56361
//   - Miguel Cut    , nº 56339

#include <tree.h>
#include <tree_skel.h>
#include <entry.h>
#include <data.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <zookeeper/zookeeper.h>

/* Estrutura que guarda informação sobre o node(server) atual e o próximo
*/
struct rtree_t {
    zhandle_t* handler;
    char* zMyNode;
    char* zNextNode;
    int nextSocket;
};

typedef struct String_vector zoo_string;

/* Esta função:
 * - Obtém o descritor da ligação (socket) da estrutura rtree_t;
 * - De-serializa a mensagem contida em msg;
 * - Serializa a mensagem de resposta;
 * - Envia a mensagem serializada para o cliente.
 */
void send_receive(MessageT *msg);

/* Função que obtém o endereço ip da interface de rede
principal do computador
*/
int get_computer_ip(char** buffer);

/* Função que, dado o endereço de um servidor ZooKeeper,
estabelece conexão com o mesmo
*/
int zookeeper_connect(char* host, char* sv_port);

/* Função que faz a conexão com o próximo server (caso exista)
da lista de servers do ZooKeeper
*/
int connect_to_server(char* address);

/* Função que ordena uma dada lista
*/
void sort_list(zoo_string** children_list);

/* FUnção que propaga um pedido ao próximo server
*/
void propagate_request(struct request_t* request);

/* Função que é chamada quando existe uma alteração num dado node
*/
void child_watcher(zhandle_t *zh, int type, int state, const char *zpath, void *watcher_ctx);

/* Função que é chamada quando existem alterações
na conexão entre o server e o ZooKeeper
*/
void watcher_server(zhandle_t *zh, int type, int state, const char *path, void *watcherCtx);

/* Função que insere na árvore um novo elemento entry
*  cujos campos se encontram na mensagem dada como argumento
*/
int put(char* key, struct data_t* data);

/* Função que devolve o valor associado à chave contida
*  na mensagem dada como argumento 
*/
int get(MessageT *msg);

/* Função que apaga o elemento da árvore cuja chave
*  é a chave que está contida na mensagem dada como argumento
*/
int del(char* key);

/* Função que devolve o tamanho da árvore
*/
int size(MessageT* msg);

/* Função que devolve a altura da árvore
*/
int height(MessageT* msg);

/* Função que devolve o conjunto de todas as chaves da árvore
*/
int get_keys(MessageT* msg);

/* Função que devolve todos os valores da árvore
*/
int get_values(MessageT* msg);

/* Função que adiciona um pedido à fila de pedidos
*/
int queue_add(MessageT* msg);