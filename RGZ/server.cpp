#include <iostream>
#include <exception>
#include <list>
#include <unistd.h>
#include <string.h>
#include <string>

#include <errno.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include <sys/types.h>

#include "sctp_utils.h"
#include "questions.hpp"

using namespace std;

#define MAX_CLIENTS 4
#define MAX_PLAYERS 2

class sock_err: public exception
{
    virtual const char* what() const throw()
    {
        return "Socket haven't been initialized";
    }
} sock_not_init;

class proc_err: public exception
{
    virtual const char* what() const throw()
    {
        return "Game process got an error";
    }
} game_proc_error;

class SCTPServer
{
private:
    int sock;
    char message[BUFFSIZE];
    struct msghdr msghdr;

    int init_socket(int *sock, struct sockaddr_in *addr, socklen_t *size)
    {
        if ((*sock = socket(PF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) == -1)
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
        return 0;
    }

public:
    
    SCTPServer(int port, const char *addr = "0.0.0.0")
    {
        struct sockaddr_in ss;
        socklen_t addr_len = sizeof(struct sockaddr_in);
        ss.sin_family = AF_INET;
        ss.sin_addr.s_addr = inet_addr(addr);
        ss.sin_port = htons(port);

        if (init_socket(&sock, &ss, &addr_len))
        {
            throw sock_not_init;
        }

        cout << "Server was inited at " << addr << ":" << port << "\n";

        if (listen(sock, MAX_CLIENTS) < 0)
        {
            throw sock_not_init;
        }

        struct sctp_initmsg initmsg;
        createInitMsg(&initmsg, 10, 10, 0, 0);
        if (setsockopt(sock, IPPROTO_SCTP, SCTP_INITMSG, &initmsg, sizeof(initmsg)) < 0)
        {
            throw sock_not_init;
        }

        int on = 1;
        if (setsockopt(sock, IPPROTO_SCTP, SCTP_RECVRCVINFO, &on, sizeof(on)) < 0) {
            throw sock_not_init;
        }
    }

    struct msghdr *SCTPRecv()
    {
        createMessageHdrRcv(&msghdr, &message, sizeof(message));
        bzero(message, sizeof(message));

        if (recvmsg(sock, &msghdr, 0) < 0)
        {
            return NULL;
        }

        return &msghdr;
    }

    int SCTPSendToAll(const char* message, int msg_len)
    {
        struct sctp_sndrcvinfo sinfo;
        createSndRcvInfo(&sinfo, 1, SCTP_SENDALL, 0, 0, 0);

        cout<<msg_len<<endl;

        createMessageHdrSndRcv(&msghdr,
                               NULL,
                               &sinfo,
                               NULL,
                               0,
                               (void *) message,
                               msg_len + 1);

        return sendmsg(sock, &msghdr, 0);
    }

    // struct msghdr *getHdr()
    // {
    //     return &msghdr;
    // }

    ~SCTPServer()
    {
        close(sock);
    }
};

class Game_proc
{
    struct Players
    {
        sctp_assoc_t assoc;
        string *name;
        int score;
    };

    enum Stages {
        GAME_INIT = 0,
        GAME_START,
        GAME_END,
        GAME_IDLE
    };

private:
    
    string **qsts;

    SCTPServer *server;
    Stages stage;
    Players players[MAX_PLAYERS];
    int round;

public:

    Game_proc()
    {
        server = new SCTPServer(10000);
        stage = GAME_INIT;
        round = 0;

        qsts = new string* [MAX_ROUNDS];
        for (int i = 0; i < MAX_ROUNDS; i++)
        {
            qsts[i] = new string [2];

            qsts[i][0] = Questions[i][0];
            qsts[i][1] = Questions[i][1];
        }
    }

    ~Game_proc()
    {

    }

    int Player_init(struct msghdr *msghdr, int player_cnt)
    {
        struct cmsghdr *cmsg;
        struct sctp_rcvinfo *rinfo;

        if(msghdr->msg_controllen > 0)
        {
            cmsg = CMSG_FIRSTHDR(msghdr);
            if (cmsg->cmsg_type = SCTP_RCVINFO)
            {
                rinfo = (struct sctp_rcvinfo *) CMSG_DATA(cmsg);
                
                //fflush(stdout);
            }
        }
        else
        {
            return -1;
        }

        players[player_cnt].assoc = rinfo -> rcv_assoc_id;
        players[player_cnt].name = new string((char *)msghdr->msg_iov->iov_base);
        cout << *players[player_cnt].name << " has been conected\n";
        players[player_cnt].score = 0;

        return 0;
    }

    int Player_proc(struct msghdr *msghdr, int scores)
    {
        struct cmsghdr *cmsg;
        struct sctp_rcvinfo *rinfo;
        int *score = NULL;

        if(msghdr->msg_controllen > 0)
        {
            cmsg = CMSG_FIRSTHDR(msghdr);
            if (cmsg->cmsg_type = SCTP_RCVINFO)
            {
                rinfo = (struct sctp_rcvinfo *) CMSG_DATA(cmsg);
            }
            else
            {
                return -1;
            }
        }
        else
        {
            return -1;
        }

        for (int i = 0; i < MAX_PLAYERS; i++)
        {
            if (rinfo -> rcv_assoc_id == players[i].assoc)
            {
                score = &players[i].score;
            }
        }

        *score += scores;

        return 0;
    }

    int Proc()
    {
        struct msghdr *msghdr;
        int mod = 0;

        switch(stage)
        {
        case GAME_INIT:
            msghdr = (server->SCTPRecv());
            if(Player_init(msghdr, 0))
            {
                return -1;
            }

            msghdr = (server->SCTPRecv());
            if(Player_init(msghdr, 1))
            {
                return -1;
            }
            stage = GAME_START;
            break;

        case GAME_START:
            server->SCTPSendToAll(qsts[round][0].c_str(), qsts[round][0].length());

            for (int i = 0; i < MAX_PLAYERS;)
            {
                msghdr = (server->SCTPRecv());
                if ((msghdr->msg_flags & (MSG_EOR | MSG_NOTIFICATION)) != 0)
                {
                    continue;
                }
                if (!strcmp((char *)msghdr->msg_iov->iov_base, qsts[round][1].c_str()))
                {
                    cout<<"Right answ "<< 200/(i+1-mod) <<endl;
                    Player_proc(msghdr, 200/(i+1-mod));
                }
                else
                {
                    cout<<"Wrong answ "<< (char *)msghdr->msg_iov->iov_base << " "<< qsts[round][1].c_str()<<endl;
                    mod++;
                }
                i++;
            }

            if (++round >= MAX_ROUNDS) stage = GAME_END;
            break;
        case GAME_END:
        {
            string finish("--1");
            for (int i = 0; i < MAX_PLAYERS; i++)
            {
                finish += *players[i].name + " " + to_string(players[i].score) + " ";
            }
            cout << finish.c_str();
            fflush(stdout);
            server->SCTPSendToAll(finish.c_str(), finish.length());
            stage = GAME_IDLE;
            break;
        }
        case GAME_IDLE:
            break;
        }

        return 0;
    }

    Stages GetState()
    {
        return stage;
    }

};

int main()
{
    try
    {
        Game_proc *game = new Game_proc();
        fflush(stdout);

        while(1)
        {
            if (game->Proc()) throw game_proc_error;
        }
    }
    catch(exception& e)
    {
        cout << e.what() << '\n';
    }


    return 0;
}