#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/select.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <errno.h>

#define max(a, b) ((a > b) ? a : b)

#define SERVER_IP INADDR_ANY
#define SERVER_PORT 16000
#define MAX_CLIENTS 10
#define MSG_LEN 10

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

int reg_select(int *sock_arr, int size, int reged)
{
    int i = -1;

    while (sock_arr[++i] != -1 && i < size);

    if (i < size)
    {
        printf("Reged: i = %d, sock = %d\n", i, reged);
        sock_arr[i] = reged;
        return i;
    }

    return -1;
}

int unreg_select(int *sock_arr, int size, int unreged)
{
    int i = -1;

    while (sock_arr[++i] != unreged && i < size);

    if (i < size)
    {
        printf("Unreged: i = %d, sock %d\n", i, unreged);
        sock_arr[i] = -1;
        return i;
    }

    return -1;
}

int main()
{
    int server_sock;
    int client_socks[MAX_CLIENTS];
    struct sockaddr_in server_addr;
    socklen_t server_size = sizeof(server_addr);
    fd_set rfds;
    struct timeval sel_timeout;
    int sock_error = 0;
    socklen_t errlen = sizeof (sock_error);
    int sock_retval;
    int max_socket;
    int tmp_sock;
    char message[MSG_LEN];
    int i_sock;

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = SERVER_IP;
    server_addr.sin_port = htons(SERVER_PORT);

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        client_socks[i] = -1;
    }

    if (init_socket(&server_sock, &server_addr, &server_size))
    {
        printf("Err sock\n");
        return -1;
    }
    listen(server_sock, MAX_CLIENTS);

    while(1)
    {
        sel_timeout.tv_sec = 0;
        sel_timeout.tv_usec = 100;

        FD_ZERO(&rfds);
        FD_SET(server_sock, &rfds);
        max_socket = server_sock;


        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (client_socks[i] >= 0)
            {
                sock_retval = getsockopt (client_socks[i], SOL_SOCKET, SO_ERROR, &sock_error, &errlen);
                //printf("Checking %dth client: ret = %d, err = %d\n", i, sock_retval, sock_error);
                if (sock_retval || sock_error)
                {
                    shutdown(client_socks[i], SHUT_RDWR);
                    if ((i_sock = unreg_select(client_socks, MAX_CLIENTS, client_socks[i])) < 0)
                    {
                        printf("Unreg err\n");
                        return -2;
                    }
                    printf("Deleted connection with %dth client: %d\n", i+1, client_socks[i]);
                    continue;
                }

                FD_SET(client_socks[i], &rfds);
                max_socket = max(client_socks[i], max_socket);
            }
        }

        select(max_socket+1, &rfds, 0, 0, &sel_timeout);

        if (FD_ISSET(server_sock, &rfds))
        {
            if ((tmp_sock = accept(server_sock, NULL, NULL)) >= 0)
            {
                if ((i_sock = reg_select(client_socks, MAX_CLIENTS, tmp_sock)) < 0)
                {
                    printf("Reg err");
                    return -2;
                }
                printf("Accpeted connection with %dth client: %d\n", i_sock, client_socks[i_sock]);
            }
            else
            {
                printf("Accepting error: %s\n", strerror(errno));
                return -5;
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (FD_ISSET(client_socks[i], &rfds))
            {
                recv(client_socks[i], message, MSG_LEN, 0);
                printf("Message is got from %d client: %d\n", i, atoi(message));
                if (atoi(message) == -1)
                {
                    if ((i_sock = unreg_select(client_socks, MAX_CLIENTS, client_socks[i])) < 0)
                    {
                        printf("Unreg err\n");
                        return -2;
                    }
                    shutdown(client_socks[i], SHUT_RDWR);
                    printf("Deleted connection with %dth client: %d\n", i+1, client_socks[i]);
                }
            }
        }

    }


    return 0;
}