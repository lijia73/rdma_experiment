#include "common.h"

// global config for the server
struct config_t config = {
    NULL,
    NULL,
    19875,
    1,
    0
};

void main() {
    struct resources res;
    int rc = 1;
    char temp_char;
    
    // init resources
    resources_init(&res);
    rc = resources_create(&res);
    printf("resource init: %d\n", rc);
    connect_qp(&res);

    post_receive(&res);
    sock_sync_data(res.sock, 1, "A", &temp_char);
    if (poll_completion(&res)) {
        return;
    }
    fprintf(stdout, "Message from clienta: %s", res.buf);
    
    strcpy(res.buf, "hello from serverb!\n");
    sock_sync_data(res.sock, 1, "B", &tempChar);
    post_send(&res, IBV_WR_SEND);
    if (poll_completion(&res)) {
        return;
    }
    return;
}