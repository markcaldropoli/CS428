/*--------------------------------------------------------------------*/
/* functions to connect clients and server */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <netdb.h>
#include <time.h> 
#include <errno.h>

#include <stdlib.h>

#define MAXNAMELEN 256
/*--------------------------------------------------------------------*/

/*----------------------------------------------------------------*/
/* prepare server to accept requests
 returns file descriptor of socket
 returns -1 on error
 */
int startserver() {
    int sd; /* socket descriptor */

    char * servhost; /* full name of this host */
    ushort servport; /* port assigned to this server */

    /*
     FILL HERE
     create a TCP socket using socket()
     */
    sd = socket(AF_INET, SOCK_STREAM, 0);
    if(sd == -1) {
        perror("cannot create socket");
        return 0;
    }

    /*
     FILL HERE
     bind the socket to some port using bind()
     let the system choose a port
     */
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = 0;
    sa.sin_addr.s_addr = htonl(INADDR_ANY);

    int bindrv = bind(sd, (struct sockaddr *) &sa, sizeof(sa));
    if (bindrv == -1) {
        perror("cannot bind");
        close(sd);
        return 0;
    }

    /* we are ready to receive connections */
    listen(sd, 5);

    /*
     FILL HERE
     figure out the full host name (servhost)
     use gethostname() and gethostbyname()
     full host name is remote**.cs.binghamton.edu
     */
    servhost = malloc(MAXNAMELEN);
    int hostrv = gethostname(servhost,MAXNAMELEN);
    if(hostrv == -1) {
        perror("cannot get host name");
        return 0;
    }

    struct hostent* host = gethostbyname(servhost);
    if(!host) {
        fprintf(stderr, "could not get address of %s\n", host);
        return 0;
    }

    memcpy(&sa.sin_addr.s_addr, host->h_addr, host->h_length);
    memcpy(servhost, host->h_name, MAXNAMELEN);

    /*
     FILL HERE
     figure out the port assigned to this server (servport)
     use getsockname()
     */
    struct sockaddr_in sa2;
    memset(&sa2,0,sizeof(sa2));

    socklen_t* len = (socklen_t *) sizeof(sa2);
    if(getsockname(sd, (struct sockaddr *) &sa2, (socklen_t *) &len) < 0) {
        perror("getsockname failed");
        return 0;
    }

    servport = ntohs(sa2.sin_port);

    /* ready to accept requests */
    printf("admin: started server on '%s' at '%hu'\n", servhost, servport);
    free(servhost);
    return (sd);
}
/*----------------------------------------------------------------*/

/*----------------------------------------------------------------*/
/*
 establishes connection with the server
 returns file descriptor of socket
 returns -1 on error
 */
int hooktoserver(char *servhost, ushort servport) {
    int sd; /* socket descriptor */

    ushort clientport; /* port assigned to this client */

    /*
     FILL HERE
     create a TCP socket using socket()
     */
    sd = socket(AF_INET, SOCK_STREAM, 0);
    if(sd < 0) {
        perror("cannot create socket");
        return 0;
    }

    /*
     FILL HERE
     connect to the server on 'servhost' at 'servport'
     use gethostbyname() and connect()
     */
    struct hostent* host = gethostbyname(servhost);
    if(!host) {
        fprintf(stderr, "could not get address of %s\n", host);
        return 0;
    }

    struct sockaddr_in sa;
    memset((char*) &sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(servport); 
    memcpy((void *) &sa.sin_addr.s_addr, host->h_addr, host->h_length);

    if(connect(sd, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
        perror("connect failed");
        return 0;
    }

    /*
     FILL HERE
     figure out the port assigned to this client
     use getsockname()
     */
    socklen_t* len = (socklen_t *) sizeof(sa);
    if(getsockname(sd, (struct sockaddr *) &sa, (socklen_t *) &len) < 0) {
        perror("getsockname failed");
        return 0;
    }
    clientport = ntohs(sa.sin_port);

    /* succesful. return socket descriptor */
    printf("admin: connected to server on '%s' at '%hu' thru '%hu'\n", servhost, servport, clientport);
    printf(">");
    fflush(stdout);
    return (sd);
}
/*----------------------------------------------------------------*/

/*----------------------------------------------------------------*/
int readn(int sd, char *buf, int n) {
    int toberead;
    char * ptr;

    toberead = n;
    ptr = buf;
    while (toberead > 0) {
        int byteread;

        byteread = read(sd, ptr, toberead);
        if (byteread <= 0) {
            if (byteread == -1)
                perror("read");
            return (0);
        }

        toberead -= byteread;
        ptr += byteread;
    }
    return (1);
}

char *recvtext(int sd) {
    char *msg;
    long len;

    /* read the message length */
    if (!readn(sd, (char *) &len, sizeof(len))) {
        return (NULL);
    }
    len = ntohl(len);

    /* allocate space for message text */
    msg = NULL;
    if (len > 0) {
        msg = (char *) malloc(len);
        if (!msg) {
            fprintf(stderr, "error : unable to malloc\n");
            return (NULL);
        }

        /* read the message text */
        if (!readn(sd, msg, len)) {
            free(msg);
            return (NULL);
        }
    }

    /* done reading */
    return (msg);
}

int sendtext(int sd, char *msg) {
    long len;

    /* write lent */
    len = (msg ? strlen(msg) + 1 : 0);
    len = htonl(len);
    write(sd, (char *) &len, sizeof(len));

    /* write message text */
    len = ntohl(len);
    if (len > 0)
        write(sd, msg, len);
    return (1);
}
/*----------------------------------------------------------------*/

