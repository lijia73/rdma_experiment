#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <getopt.h>
#include <rdma/rdma_cma.h>
#include <rdma/rdma_verbs.h>

static char *server = "127.0.0.1";
static char *port = "7471";

struct rdma_cm_id *id;
struct ibv_mr *mr, *send_mr;
int sendFlags;
uint8_t send_msg[16];
uint8_t recv_msg[16];

int run();

int main(int argc, char **argv) {
    int op, ret;

    while ((op = getopt(argc, argv, "s:p:")) != -1) {
        switch (op)
        {
        case 's':
            server = optarg;
            break;
        case 'p':
            port = optarg;
            break;
        default:
            printf("usage: %s\n", argv[0]);
            printf("\t[-s server address]\n");
            printf("\t[-p port number]\n");
            exit(1);
        }
    }
    printf("rdma client start\n");
    ret = run();
    printf("rdma client end %d\n", ret);
    return ret;
}

int run() {
    struct rdma_addrinfo hints, *res;
    struct ibv_qp_init_attr attr;
    struct ibv_wc wc;
    int ret;

    memset(&hints, 0, sizeof(hints));
    hints.ai_port_space = RDMA_PS_TCP;
    ret = rdma_getaddrinfo(server, port, &hints, &res);
    if (ret) {
        printf("rdma_getaddrinfo: %s\n", gai_strerror(ret));
        goto out;
    }

    memset(&attr, 0, sizeof(attr));
    attr.cap.max_send_wr = attr.cap.max_recv_wr = 1;
    attr.cap.max_send_sge = attr.cap.max_recv_sge = 1;
    attr.cap.max_inline_data = 16;
    attr.qp_context = id;
    attr.sq_sig_all = 1;
    ret = rdma_create_ep(&id, res, NULL, &attr);

    if (attr.cap.max_inline_data >= 16)
        sendFlags = IBV_SEND_INLINE;
    else
        printf("rdma_client: device doesn't support IBV_SEND_INLINE, "
		       "using sge sends\n");
    
    if (ret) {
        perror("rdma_create_ep");
        goto outFreeAddrinfo;
    }

    mr = rdma_reg_msgs(id, recv_msg, 16);
    if (!mr) {
        perror("rdma_reg_msgs for recv_msg");
        ret = -1;
        goto outDestroyEP;
    }
    if ((sendFlags & IBV_SEND_INLINE) == 0) {
        send_mr = rdma_reg_msgs(id, send_msg, 16);
        if (!send_mr) {
            perror("rdma_reg_msgs for send_msg");
            return -1;
            goto outDeregRecv;
        }
    }

    ret = rdma_post_recv(id, NULL, recv_msg, 16, mr);
    if (ret) {
        perror("rdma_post_recv");
        goto outDeregSend;
    }

    ret = rdma_connect(id, NULL);
    if (ret) {
        perror("rdma_connect");
        goto outDeregSend;
    }

    strcpy(send_msg, "client: hello");
    ret = rdma_post_send(id, NULL, send_msg, 16, send_mr, sendFlags);
    if (ret) {
        perror("rdma_post_send");
        goto outDisconnect;
    }

    while ((ret = rdma_get_send_comp(id, &wc)) == 0);
    if (ret < 0) {
        perror("rdma_get_send_comp");
        goto outDisconnect;
    }

    while ((ret = rdma_get_recv_comp(id, &wc)) == 0);
    if (ret < 0) {
        perror("rdma_get_recv_comp");
    } else
    {
        ret = 0;
    }
    printf("message received: %s\n", recv_msg);

outDisconnect:
    rdma_disconnect(id);
outDeregSend:
    if ((sendFlags & IBV_SEND_INLINE) == 0)
        rdma_dereg_mr(send_mr);
outDeregRecv:
    rdma_dereg_mr(mr);
outDestroyEP:
    rdma_destroy_ep(id);
outFreeAddrinfo:
    rdma_freeaddrinfo(res);
out:
    return ret;
}