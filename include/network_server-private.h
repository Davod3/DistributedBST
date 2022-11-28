// Ficheiro desenvolvido pelo grupo 7, cujos elementos são:
//   - André Dias    , nº 55314
//   - David Pereira , nº 56361
//   - Miguel Cut    , nº 56339

/*Função que dado um set de file descriptors, um file descriptor e
 *o número de file descriptors no set, remove o file descriptor
 *do set de file descriptors
*/
void remove_fd(struct pollfd** desc_set, int fd, int size);