#include "common.h"

// global config for the server
struct config_t config = {
    NULL,
    NULL,
    19875,
    1,
    0};

void main()
{
    struct resources res;
    char tempChar;

    // init resources
    resources_init(&res);
    resources_create(&res);
    connect_qp(&res);

    post_receive(&res);
    sock_sync_data(res.sock, 1, "A", &tempChar);
    if (poll_completion(&res))
    {
        return;
    }
    fprintf(stdout, "Message from clienta: %s", res.buf);

    config.server_name = "128.110.96.112";
    resources_init(&res);
    resources_create(&res);
    connect_qp(&res);

    strcpy(res.buf, "hello from serverb!\n");
    sock_sync_data(res.sock, 1, "B", &tempChar);
    post_send(&res, IBV_WR_SEND);
    if (poll_completion(&res))
    {
        return;
    }
    return;
}