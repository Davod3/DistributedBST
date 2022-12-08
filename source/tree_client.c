// Ficheiro desenvolvido pelo grupo 7, cujos elementos são:
//   - André Dias    , nº 55314
//   - David Pereira , nº 56361
//   - Miguel Cut    , nº 56339

#include "inet.h"
#include <stdio.h>
#include <string.h>
#include "client_stub.h"
#include "entry.h"
#include "data.h"
#include "client_stub-private.h"
#include "tree_client-private.h"
#include <signal.h>
#include "network_client.h"

struct rtree_t* rtree;

//Empty on purpose
void sigHandler() {}

int main(int argc, char const *argv[]) {

    if(argc < 2) {
        printf("Incorrect usage. Try ./tree-client <hostname:port>\n");
        return -1;
    }

    rtree = rtree_connect(argv[1]);
    if (rtree == NULL) {
        return -1;
    }

    char comando[50];

    printf("Insira o comando: \n");

        struct sigaction sa;
        sa.sa_handler = sigHandler;
        sa.sa_flags = 0;
        sigemptyset(&sa.sa_mask); //apaga a máscara (nenhum sinal é bloqueado)

        if (sigaction(SIGINT, &sa, NULL) == -1 || sigaction(SIGPIPE, &sa, NULL) == -1) {
            perror("main:");
            exit(-1);
        }

    while(fgets(comando, 50, stdin) != NULL) {

        //obtem a string dividida (strtok) e põe num array
        char* token = strtok(comando, " ");

        char* comandos[50];
        comandos[0] = NULL;
        comandos[1] = NULL;
        comandos[2] = NULL;

        int pointer = 0;

        while (token != NULL) {
            comandos[pointer] = token;

            token = strtok(NULL, " ");
            trim(comandos[pointer]);
            pointer++;
        }

        if (strcmp(comandos[0],"put") == 0) {

            if(comandos[1] == NULL || comandos[2] == NULL) {
                printf("Incorrect usage. Try put <key> <entry>\n");
                continue;
            }

            char *data = malloc(1024*sizeof(char));
            
            for (int i = 0; i < 1024; i++) {
                data[i] = '\0';
            }

            char* ws = " ";
            for(int i = 2; i < pointer; i++) {
                strcat(data, comandos[i]);
                strcat(data, ws);
            }

            trim(data);

            struct data_t* value = data_create2(strlen(data) * sizeof(char) + 1, data);
            struct entry_t* entry = entry_create(comandos[1], value);
            int result = rtree_put(rtree, entry);
            if(result == -1) {
                printf("Failed to put entry on tree\n");
                free(data);
                continue;
            } 

            printf("Put request sucessfuly added to queue with id %d .\n", result);

            free(data);
            free(value);
            free(entry);
        }
        else if (strcmp(comandos[0], "get") == 0) {

            if(comandos[1] == NULL) {
                printf("Incorrect usage. Try get <key>\n");
                continue;
            }

            struct data_t* element = rtree_get(rtree, comandos[1]);

            if(element == NULL) {
                printf("Error getting element from tree\n");
            } else {
                printf("Element: %s\n", (char*)element->data);
                data_destroy(element);
            }
        } 
        else if (strcmp(comandos[0], "del") == 0) {
            
            if(comandos[1] == NULL) {
                printf("Incorrect usage. Try del <key>\n");
                continue;
            }
            int result = rtree_del(rtree, comandos[1]);
            if(result == -1) {
                printf("Error deleting entry.\n");
            } else {
                 printf("Delete request sucessfuly added to queue with id %d .\n", result);
            }
        }
        else if (strcmp(comandos[0], "size") == 0) {
            
            int size = rtree_size(rtree);
            printf("Size: %d.\n", size);

        }
        else if (strcmp(comandos[0], "height") == 0) {
            
            int size = rtree_height(rtree);
            printf("Height: %d.\n", size);
        }
        else if (strcmp(comandos[0], "getkeys") == 0) {
            
            char** keys = rtree_get_keys(rtree);

            if(keys != NULL){

                if(keys[0] == NULL) {
                    printf("Error getting keys from tree.\n");
                    free(keys);
                } else {

                    int counter = 0;

                    while(keys[counter] != NULL) {

                        printf("Key: %s\n", keys[counter]);
                        free(keys[counter]);
                        counter++;
                    }

                free(keys);
                }

            }

        }
        else if (strcmp(comandos[0], "getvalues") == 0) {
            
            void** values = rtree_get_values(rtree);

            if(values[0] == NULL) {
                printf("Error getting values from tree\n");
                free(values);
            } else {

                int counter = 0;
                while(values[counter] != NULL) {

                    printf("Value: %s\n", (char*)values[counter]);
                    free(values[counter]);
                    counter++;
                }

                free(values);
            }
        }else if (strcmp(comandos[0], "verify") == 0) {

            if(comandos[1] == NULL) {
                printf("Incorrect usage. Try verify <op_n>\n");
                continue;
            }
            int result = rtree_verify(rtree, atoi(comandos[1]));
            if(result == 0){
                printf("A operação não foi executada. Encontra-se na lista de espera\n");
            }else if(result == 1){
                printf("A operação já foi executada.\n");
            }else{
                printf("Não é possível verificar essa operação.\n");
            }
        } else if (strcmp(comandos[0], "quit") == 0) {
                int state = rtree_disconnect(rtree);
                exit(state);
        }
        else {
            printf("Comando não existente: \n");
            printf("    put <key> <values>\n");
            printf("    get <key>\n");
            printf("    del <key>\n");
            printf("    size\n");
            printf("    height\n");
            printf("    getkeys\n");
            printf("    getvalues\n");
            printf("    verify <op_n>\n");
            printf("    quit\n");
        }
        
        //Pede de novo outro comando
        printf("Insira outro comando: \n");
    }

    return 0;
}

void trim(char * str){

    int pos = 0, indice = -1;

    while(str[pos] != '\0')
    {
        if(str[pos] != ' ' && str[pos] != '\t' && str[pos] != '\n')
        {
            indice = pos;
        }

        pos++;
    }

    str[indice + 1] = '\0';
}

