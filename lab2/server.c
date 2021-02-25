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


#define BUFF_SIZE 32

#define handle_error(msg) \
    do { perror(msg); exit(1); } while (0)


int handler(int);

int main(int argc, char const *argv[])
{
    pthread_t th;
    pthread_attr_t ta;

    int sock_clnt;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    int flag = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int));
        
    struct sockaddr_in ss;
    ss.sin_family = AF_INET;
    ss.sin_addr.s_addr = inet_addr("127.0.0.1"); // htons(INADDR_ANY);

    if (bind(sock, (struct sockaddr *)&ss, sizeof(struct sockaddr_in)) == -1) handle_error("bind");

    int len = sizeof(ss);
    getsockname(sock, (struct sockaddr *)&ss, &len);

    printf("port is %d\n", ntohs(ss.sin_port));

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
