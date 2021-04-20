#include <stdio.h> // printf
#include <stdlib.h> // exit
#include <unistd.h> // close
#include <errno.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFF_SIZE 32
#define PORT 16000

#define handle_error(msg) \
    do { perror(msg); exit(1); } while (0)

void reaper(int sig)
{
    int status;
    while (wait3(&status, WNOHANG, (struct rusage *)0) >= 0);
}

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
    int sock_clnt;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    int flag = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int));
        
    struct sockaddr_in ss;
    socklen_t addr_len = sizeof(struct sockaddr_in);
    ss.sin_family = AF_INET;
    ss.sin_addr.s_addr = INADDR_ANY;// htons(INADDR_ANY);
    ss.sin_port = htons(PORT);

    if (init_socket(&sock, (struct sockaddr *)&ss, &addr_len)) handle_error("bind");

    int len = sizeof(ss);
    getsockname(sock, (struct sockaddr *)&ss, &len);

    if (listen(sock, 5) == -1) handle_error("listen");

    signal(SIGCHLD, reaper);
    for (;;) {
        if ((sock_clnt = accept(sock, NULL, 0)) < 0) {
            printf("err\n");
        }
        if ((fork()) == 0) {
            close(sock);
            int bred;
            char buff[BUFF_SIZE];

            while ((bred = read(sock_clnt, buff, BUFF_SIZE)) > 0) {
                write(STDIN_FILENO, buff, bred);
                write(STDIN_FILENO, "\n", 1);
            }

            close(sock_clnt);
            exit(0);
        } else {
            close(sock_clnt);
        }
    }

    close(sock);

    return 0;
}
