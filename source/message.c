// Ficheiro desenvolvido pelo grupo 7, cujos elementos são:
//   - André Dias    , nº 55314
//   - David Pereira , nº 56361
//   - Miguel Cut    , nº 56339

#include <message-private.h>

int write_all(int sock, void* buf, int len) {
    
    int bufsize = len;

    while(len>0) {

        int res = write(sock, buf, len);

        if(res<0) {
            perror("write failed");
            close(sock);
            return res;
        }
        buf += res;
        len -= res;
    } 

    return bufsize;
}

int read_all(int sock, void* buf, int len){

    int bufsize = len;

    while (len>0) {
        
        int nbytes = read(sock, buf, len);

        if(nbytes<=0) {
            perror("Read failed\n");
            close(sock);
            return nbytes;
        }

        buf+=nbytes;
        len-=nbytes;

    }
    
    return bufsize;
}