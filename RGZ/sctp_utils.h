#define BUFFSIZE 256

void createInitMsg(struct sctp_initmsg *initmsg,
                   u_int16_t num_ostreams,
                   u_int16_t max_instreams,
                   u_int16_t max_attempts,
                   u_int16_t max_init_timeo);

void createSndRcvInfo(struct sctp_sndrcvinfo *sinfo,
                      uint32_t ppid,
                      uint16_t flags,
                      uint16_t stream_no,
                      uint32_t timetolive,
                      uint32_t context);

void createMessageHdrSndRcv(struct msghdr *outmsghdr,
                             struct sctp_initmsg *initmsg,
                             struct sctp_sndrcvinfo *sinfo,
                             struct sockaddr *to,
                             socklen_t tolen,
                             void *msg,
                             size_t len);

void createMessageHdrRcv(struct msghdr *msg,
                         void *message,
                            size_t mlen);