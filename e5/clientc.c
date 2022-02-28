#include "common.h"

struct config_t config = {
    NULL,
    "128.110.96.113",
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
    
    post_receive(&res);
    sock_sync_data(res.sock, 1, "B", &tempChar);
    if (poll_completion(&res)) {
        return;
    }
    fprintf(stdout, "Message from clientb: %s", res.buf);

    strcpy(res.buf, "hello from serverc!\n");
    sock_sync_data(res.sock, 1, "C", &tempChar);
    post_send(&res, IBV_WR_SEND);
    if (poll_completion(&res)) {
        return;
    }
    return;
}