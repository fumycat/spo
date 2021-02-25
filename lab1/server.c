#include <stdio.h> // printf
#include <stdlib.h> // exit
#include <unistd.h> // close

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFF_SIZE 32

#define handle_error(msg) \
    do { perror(msg); exit(1); } while (0)

void reaper(int sig)
{
	int status;
	while (wait3(&status, WNOHANG, (struct rusage *)0) >= 0);
}


int main(int argc, char const *argv[])
{
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
