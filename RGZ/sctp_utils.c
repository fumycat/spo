#include <netinet/sctp.h>
//#include <netinet/sctp_uio.h>

#include <string.h>

#include "sctp_utils.h"

#define SCTP_CONTROL_VEC_SIZE_RCV  16384

void createInitMsg(struct sctp_initmsg *initmsg,
                   u_int16_t num_ostreams,
                   u_int16_t max_instreams,
                   u_int16_t max_attempts,
                   u_int16_t max_init_timeo) {

    bzero(initmsg, sizeof(*initmsg));
    initmsg->sinit_num_ostreams = num_ostreams;
    initmsg->sinit_max_instreams = max_instreams;
    initmsg->sinit_max_attempts = max_attempts;
    initmsg->sinit_max_init_timeo = max_init_timeo;

}

void createSndRcvInfo(struct sctp_sndrcvinfo *sinfo,
                      uint32_t ppid,
                      uint16_t flags,
                      uint16_t stream_no,
                      uint32_t timetolive,
                      uint32_t context) {

    bzero(sinfo, sizeof(*sinfo));
    sinfo->sinfo_ppid = ppid;
    sinfo->sinfo_flags = flags;
    sinfo->sinfo_stream = stream_no;
    sinfo->sinfo_timetolive = timetolive;
    sinfo->sinfo_context = context;
}

void createMessageHdrSndRcv(struct msghdr *outmsghdr,
                             struct sctp_initmsg *initmsg,
                             struct sctp_sndrcvinfo *sinfo,
                             struct sockaddr *to,
                             socklen_t tolen,
                             void *msg,
                             size_t len) {

    char cmsgbuf[CMSG_SPACE(sizeof(struct sctp_sndrcvinfo))];
    struct cmsghdr *cmsg;

    // server address
    outmsghdr->msg_name = to;
    outmsghdr->msg_namelen = tolen;

    // message
    struct iovec iov;
    outmsghdr->msg_iov = &iov;
    outmsghdr->msg_iovlen = 1;
    iov.iov_base = (void *) msg;
    iov.iov_len = len;

    outmsghdr->msg_control = cmsgbuf;
    outmsghdr->msg_controllen = sizeof(cmsgbuf);
    outmsghdr->msg_flags = 0;

    cmsg = CMSG_FIRSTHDR(outmsghdr);
    cmsg->cmsg_level = IPPROTO_SCTP;
    cmsg->cmsg_type = SCTP_SNDRCV;
    cmsg->cmsg_len = CMSG_LEN(sizeof(struct sctp_sndrcvinfo));

    outmsghdr->msg_controllen = cmsg->cmsg_len;
    memcpy(CMSG_DATA(cmsg), sinfo, sizeof(struct sctp_sndrcvinfo));

}

void createMessageHdrRcv(struct msghdr *msg,
                         void *message,
                            size_t mlen) {

    char cbuf[SCTP_CONTROL_VEC_SIZE_RCV];
    bzero(msg, sizeof(msg));
    bzero(cbuf, sizeof(cbuf));
    msg->msg_control = &cbuf;
    msg->msg_controllen = sizeof(cbuf);

    struct iovec iov[1];
    iov->iov_base = message;
    iov->iov_len = mlen;

    msg->msg_iov = iov;
    msg->msg_iovlen = 1;

}