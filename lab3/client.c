#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>
#include <netdb.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define handle_error(msg) \
    do { perror(msg); exit(1); } while (0)


int main(int argc, char const *argv[])
{
    struct hostent *hp, *gethostbyname();

    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == -1) handle_error("socket");

    struct sockaddr_in ss;
    bzero((char*)&ss, sizeof(ss));
    ss.sin_family = AF_INET;
    hp = gethostbyname(argv[1]);
    bcopy(hp -> h_addr, &ss.sin_addr, hp -> h_length);
    ss.sin_port = htons(atoi(argv[2]));
    int len = sizeof(ss);

    if (connect(s, (struct sockaddr *)&ss, sizeof(struct sockaddr_in)) == -1) handle_error("connect");

    for (int x = 0; x < 2; x++) {
        send(s, argv[3], 10, 0);
        sleep(atoi(argv[3]));
    }

    send(s, "-1", 10, 0);

    close(s);
    

    return 0;
}