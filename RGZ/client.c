#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>
#include <netdb.h>
#include <netinet/sctp.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "sctp_utils.h"

#define handle_error(msg) \
    do { perror(msg); exit(1); } while (0)

int main(int argc, char const *argv[])
{
    int res;
    struct hostent *hp;
    struct iovec iov;
    struct msghdr hdr;
    struct cmsghdr chdr;
    char *message = "Hi";
    char recv[BUFFSIZE];

    int sock = socket(PF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
    if (sock == -1) handle_error("socket");

    struct sockaddr_in ss;
    bzero((char*)&ss, sizeof(ss));
    ss.sin_family = AF_INET;
    hp = gethostbyname(argv[1]);
    bcopy(hp -> h_addr, &ss.sin_addr, hp -> h_length);
    ss.sin_port = htons(atoi(argv[2]));
    int len = sizeof(ss);

    struct sctp_initmsg initmsg;
    createInitMsg(&initmsg, 10, 10, 0, 0);

    res = setsockopt(sock, IPPROTO_SCTP, SCTP_INITMSG, &initmsg, sizeof(initmsg));
    if(res == -1) handle_error("error setting socket options");

    struct sctp_sndrcvinfo sinfo;
    createSndRcvInfo(&sinfo, 1, 0, 0, 0, 0);

    struct msghdr msghdr = {0};
    createMessageHdrSndRcv(&msghdr,
                           &initmsg,
                           &sinfo,
                           (struct sockaddr *) &ss,
                           sizeof(ss),
                           (void *) argv[3],
                           sizeof(argv[3]) + 1);

    ssize_t n = sendmsg(sock, &msghdr, 0);
    if(n < 0) handle_error("error sending message");

    char answer[BUFFSIZE];
    char comm [4];
    while(1)
    {
        createMessageHdrRcv(&msghdr, &recv, BUFFSIZE);
        bzero(recv, sizeof(message));
        
        if (recvmsg(sock, &msghdr, 0) < 0)
        {
            break;
        }

        memcpy(comm, recv, sizeof(comm));
        comm[3] = '\0';

        if ((msghdr.msg_flags & (MSG_NOTIFICATION)) == 0 || msghdr.msg_iov->iov_len == 0)
        {
            if (!strcmp(comm, "--1"))
            {
                printf("%s\n", (char*) msghdr.msg_iov->iov_base + 3);
                break;
            }
            printf("Question: %s\n", (char*) msghdr.msg_iov->iov_base);
        }
        else
        {
            continue;
        }

        fgets(answer, sizeof(answer), stdin);

        char *pch = strstr(answer, "\n");
        if (pch)
        {
            strncpy(pch, "\0", 1);
        }

        createSndRcvInfo(&sinfo, 1, 0, 0, 0, 0);

        createMessageHdrSndRcv(&msghdr,
                               &initmsg,
                               &sinfo,
                               (struct sockaddr *) &ss,
                               sizeof(ss),
                               (void *) answer,
                               sizeof(answer) + 1);

        ssize_t n = sendmsg(sock, &msghdr, 0);
        if(n < 0) handle_error("error sending message");

    }

    close(sock);


    return 0;
}