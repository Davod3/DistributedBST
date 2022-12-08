// Ficheiro desenvolvido pelo grupo 7, cujos elementos são:
//   - André Dias    , nº 55314
//   - David Pereira , nº 56361
//   - Miguel Cut    , nº 56339

#include <network_server.h>
#include <tree_skel.h>
#include <signal.h>
#include <tree_server-private.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char **args;
int n_args;

int main(int argc, char *argv[])
{
    
    if(argc > 2) {
        int port = atoi(argv[1]);
        args = (char**) argv;
        n_args = argc;
        printf("Binding to port: %d\n", port);

        int listening_socket = network_server_init(port);

        if(listening_socket < 0) {
            return -1;
        }

        printf("Connected!");

        struct sigaction saC;
        saC.sa_handler = (void (*) (int)) network_server_close;
        saC.sa_flags = 0;
        sigemptyset(&saC.sa_mask); //apaga a máscara (nenhum sinal é bloqueado)

        struct sigaction saR;
        saR.sa_handler = signalRestart;
        saR.sa_flags = 0;
        sigemptyset(&saR.sa_mask); //apaga a máscara (nenhum sinal é bloqueado)

        if (sigaction(SIGINT, &saC, NULL) == -1 || sigaction(SIGPIPE, &saR, NULL) == -1) {
            perror("main:");
            exit(-1);
        }

        tree_skel_init(argv[2], argv[1]);

        int result = network_main_loop(listening_socket);

        printf("Out of main loop\n");

        if(result < 0) {
            printf("Error listening\n");
            return -1;
        }

        tree_skel_destroy();

    } else {
        printf("Incorrect usage try ./tree-server <port> <n_threads>\n");
    }

    return 0;
}

void signalRestart(){
    printf("Restarting...\n");
    main(n_args, (char**)args);
}
