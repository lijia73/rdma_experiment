#include "common.h"

struct config_t config = {
    NULL,
    "10.10.10.1",
    19875,
    1,
    0
};

void main() {
    struct resources res;
    char tempChar;
    resources_init(&res);
    resources_create(&res);
    connect_qp(&res);
    // if (poll_completion(&res)) {
    //     return;
    // }
    fprintf(stdout, "Message is: %s\n", res.buf);
    sock_sync_data(res.sock, 1, "I", &tempChar);
    if (post_send(&res, IBV_WR_SEND)) {
        fprintf(stdout, "Failed to post sencond send\n");
        return;
    }
    if (poll_completion(&res)) {
        return;
    }
    if (sock_sync_data(res.sock, 1, "R", &tempChar)) {
        return;
    }
    printf("client reading from server buffer\n");
    post_send(&res, IBV_WR_RDMA_READ);
    printf("client polling completion queue\n");
    poll_completion(&res);
    fprintf(stdout, "contents of server's buffer: %s\n", res.buf);
    strcpy(res.buf, "hellow from client\n");
    printf("client writing to server buffer\n");
    post_send(&res, IBV_WR_RDMA_WRITE);
    printf("client polling completion queue\n");
    poll_completion(&res);
    sock_sync_data(res.sock, 1, "W", &tempChar);
    return;
}