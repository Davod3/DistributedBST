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