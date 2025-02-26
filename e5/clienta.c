#include "common.h"

struct config_t config = {
    NULL,
    "128.110.96.95",
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

    strcpy(res.buf, "hello from clienta!\n");
    sock_sync_data(res.sock, 1, "A", &tempChar);
    post_send(&res, IBV_WR_SEND);
    if (poll_completion(&res)) {
        return;
    }

    config.server_name = NULL;
    resources_init(&res);
    resources_create(&res);
    connect_qp(&res);

    post_receive(&res);
    sock_sync_data(res.sock, 1, "C", &tempChar);
    if (poll_completion(&res)) {
        return;
    }
    fprintf(stdout, "Message from clientc: %s", res.buf);
    
    
    return;
}