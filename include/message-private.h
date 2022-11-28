// Ficheiro desenvolvido pelo grupo 7, cujos elementos são:
//   - André Dias    , nº 55314
//   - David Pereira , nº 56361
//   - Miguel Cut    , nº 56339

#include <inet.h>

/* Função que envia toda a informação de buf através de sock
*/
int write_all(int sock, void *buf, int len);

/*Função que lê toda a informação de sock para buf
*/
int read_all(int sock, void* buf, int len);