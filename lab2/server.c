#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>
#include <netdb.h>

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>


#define BUFF_SIZE 32
#define PORT 16000

#define handle_error(msg) \
    do { perror(msg); exit(1); } while (0)


int handler(int);

int init_socket(int *sock, struct sockaddr_in *addr, socklen_t *size)
{
    if ((*sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
    {
        return -1;
    }

    while (bind(*sock, (struct sockaddr *)addr, *size) == -1)
    {
        switch (errno)
        {
        case EADDRINUSE:
        case EACCES:
            addr->sin_port = htons(htons(addr->sin_port) + 1);
            break;
        default:
            return -1;
        }
    }

    printf("Server port: %d\n", ntohs(addr->sin_port));

    return 0;
}

int main(int argc, char const *argv[])
{
    pthread_t th;
    pthread_attr_t ta;

    int sock_clnt;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    int flag = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int));
        
    struct sockaddr_in ss;
    socklen_t addr_len = sizeof(struct sockaddr_in);
    ss.sin_family = AF_INET;
    ss.sin_addr.s_addr = INADDR_ANY; // htons(INADDR_ANY);
    ss.sin_port = htons(PORT);

    if (init_socket(&sock, (struct sockaddr *)&ss, &addr_len)) handle_error("bind");

    int len = sizeof(ss);
    getsockname(sock, (struct sockaddr *)&ss, &len);

    if (listen(sock, 5) == -1) handle_error("listen");

    // *
    pthread_attr_init(&ta);
    pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);

    for (;;) {
        if ((sock_clnt = accept(sock, NULL, 0)) < 0) {
            printf("err\n");
        }
        if ((pthread_create(&th, &ta, (void*) handler, (void*)(intptr_t) sock_clnt)) < 0) {
            printf("pthread_create err\n");
        }
    }

    return 0;
}

int handler(int par)
{    
    int bred;
    char buff[BUFF_SIZE];
    int sock_clnt = (intptr_t) par;

    while ((bred = read(sock_clnt, buff, BUFF_SIZE)) > 0) {
        write(STDIN_FILENO, buff, bred);
        write(STDIN_FILENO, "\n", 1);
    }

    close(sock_clnt);
}
