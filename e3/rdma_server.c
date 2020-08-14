#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <rdma/rdma_cma.h>
#include <rdma/rdma_verbs.h>

static char *port = "7471";

struct rdma_cm_id *listenID, *id;
struct ibv_mr *mr, *sendMr;
int sendFlags;
uint8_t send_msg[16];
uint8_t recv_msg[16];

int run();

int main(int argc, char **argv) {
    int op, ret;

    while ((op = getopt(argc, argv, "p:")) != -1) {
        switch (op)
        {
        case 'p':
            port = optarg;
            break;
        default:
            printf("usage: %s\n", argv[0]);
            printf("\t[-p port_number]\n");
            exit(1);
        }
    }
    printf("rdma server start\n");
    ret = run();
    printf("rdma server end %d\n", ret);
    return ret;
}

int run(void) {
    struct rdma_addrinfo hints, *res;
    struct ibv_qp_init_attr initAttr;
    struct ibv_qp_attr qpAttr;
    struct ibv_wc wc;
    int ret;

    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = RAI_PASSIVE;
    hints.ai_port_space = RDMA_PS_TCP;
    ret = rdma_getaddrinfo(NULL, port, &hints, &res);
    if (ret) {
        printf("rdma_getaddrinfo: %s\n", gai_strerror(ret));
    }

    memset(&initAttr, 0, sizeof(initAttr));
    initAttr.cap.max_inline_data = 16;
    initAttr.cap.max_send_wr = initAttr.cap.max_recv_wr = 1;
    initAttr.cap.max_send_sge = initAttr.cap.max_recv_sge = 1;
    initAttr.sq_sig_all = 1;
    ret = rdma_create_ep(&listenID, res, NULL, &initAttr);
    if (ret) {
        perror("rdma_create_ep");
        goto outFreeAddrinfo;
    }

    ret = rdma_listen(listenID, 0);
    if (ret) {
        perror("rdma_listen");
        goto outDestroyListenEP;
    }

    ret = rdma_get_request(listenID, &id);
    if (ret) {
        perror("rdma_get_request");
        goto outDestroyListenEP;
    }

    memset(&qpAttr, 0, sizeof(qpAttr));
    memset(&initAttr, 0, sizeof(initAttr));
    ret = ibv_query_qp(id->qp, &qpAttr, IBV_QP_CAP, &initAttr);
    if (ret) {
        perror("ibv_query_qp");
        goto outDestroyAcceptEP;
    }
    if (initAttr.cap.max_inline_data >= 16)
        sendFlags = IBV_SEND_INLINE;
    else
        printf("rdma_server: device doesn't support IBV_SEND_INLINE, "
		       "using sge sends\n");

    mr = rdma_reg_msgs(id, recv_msg, 16);
    if (!mr) {
        ret = -1;
        perror("rdma_reg_msgs for recv_msg");
        goto outDestroyAcceptEP;
    }
    if ((sendFlags & IBV_SEND_INLINE) == 0) {
        sendMr = rdma_reg_msgs(id, send_msg, 16);
        if (!sendMr) {
            ret = -1;
            perror("rdma_reg_msgs for send_msg");
            goto outDeregRecv;
        }
    }

    ret = rdma_post_recv(id, NULL, recv_msg, 16, mr);
    if (ret) {
        perror("rdma_post_recv");
        goto outDeregSend;
    }

    ret = rdma_accept(id, NULL);
    if (ret) {
        perror("rdma_accept");
        goto outDeregSend;
    }

    while((ret = rdma_get_recv_comp(id, &wc)) == 0);
    if (ret < 0) {
        perror("rdma_get_recv_comp");
        goto outDisconnect;
    }
    printf("Message received: %s\n", recv_msg);

    strcpy(send_msg, "server: hello");
    ret = rdma_post_send(id, NULL, send_msg, 16, sendMr, sendFlags);
    if (ret) {
        perror("rdma_post_send");
        goto outDisconnect;
    }

    while ((ret = rdma_get_send_comp(id, &wc)) == 0);
    if (ret < 0)
        perror("rdma_get_send_comp");
    else
        ret = 0;
outDisconnect:
    rdma_disconnect(id);
outDeregSend:
    if ((sendFlags & IBV_SEND_INLINE) == 0)
        rdma_dereg_mr(sendMr);
outDeregRecv:
    rdma_dereg_mr(mr);
outDestroyAcceptEP:
    rdma_destroy_ep(id);
outDestroyListenEP:
    rdma_destroy_ep(listenID);
outFreeAddrinfo:
    rdma_freeaddrinfo(res);
    return ret;
}