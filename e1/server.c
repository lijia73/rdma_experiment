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
    post_send(&res, IBV_WR_SEND);
    if (poll_completion(&res)) {
        return;
    }
    post_receive(&res);
    sock_sync_data(res.sock, 1, "I", &temp_char);
    if (poll_completion(&res)) {
        return;
    }
    fprintf(stdout, "Message from client: %s", res.buf);
    strcpy(res.buf, "hello from server side!\n");
    if (sock_sync_data(res.sock, 1, "R", &temp_char)) {
        rc = 1;
    }

    if (sock_sync_data(res.sock, 1, "W", &temp_char)) {
        rc = 1;
    }
    fprintf(stdout, "Contents of server buffer: %s\n", res.buf);

    rc = 0;
    return rc;
}