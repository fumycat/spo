#include <stdio.h> // printf
#include <stdlib.h> // exit
#include <unistd.h> // close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>


#define handle_error(msg) \
    do { perror(msg); exit(1); } while (0)


int main(int argc, char const *argv[])
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    int flag = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int));
        
    struct sockaddr_in ss;
    ss.sin_family = AF_INET;
    ss.sin_addr.s_addr = htons(INADDR_ANY);

    if (bind(sock, (struct sockaddr *)&ss, sizeof(struct sockaddr_in)) == -1) handle_error("bind");

    getsockname(sock, (struct sockaddr *)&ss, NULL);

    printf("port is %d\n", ntohs(ss.sin_port));

    if (listen(sock, 5) == -1) handle_error("listen");

    int childpid = fork();
    int ogpid = getpid();

    printf("%d and %d\n", getpid, childpid);

    close(sock);

    return 0;
}
