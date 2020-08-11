#include "common.hpp"

struct config_t config;

int main(int argc, char **argv) {
    config.devName = NULL;
    config.serverName = "10.10.10.1";
    config.tcpPort = 19971;
    config.gidIdx = -1;
    config.ibPort = 1;

    struct resources res;
    int error = 0;
    char syncChar;

    fprintf(stdout, "client creating resources\n");

    error = createResources(&res);
    if (error) {
        fprintf(stderr, "Failed to create resources\n");
        return 0;
    }

    fprintf(stdout, "client connecting qp\n");
    error = connectQP(&res);
    if (error) {
        fprintf(stderr, "Failed to connect QP\n");
        return 0;
    }

    error = postReceive(&res);
    if (error) {
        fprintf(stdout, "Failed to post the first receive\n");
    }
    // sync after connect
    error = sockSyncData(res.sock, 1,"1", &syncChar);
    if (syncChar != '1') {
        fprintf(stdout, "Failed to sync after connect\n");
        return 0;
    }
    if (error) {
        fprintf(stdout, "Failed to sync after connecting qp\n");
        return 0;
    }

    // polling for the first message
    fprintf(stdout, "client polling cq\n");
    error = pollCQ(&res);
    if (error) {
        fprintf(stderr, "Failed to poll cq\n");
        return 0;
    }
    fprintf(stdout, "****\nMessage received: %s\nimm data: %d\n****\n", res.buf, res.immData);

    // syncing for receive ready
    sockSyncData(res.sock, 1, "2", &syncChar);
    if (syncChar != '2') {
        fprintf(stdout, "Failed to sync for message 2\n");
        return 0;
    }
    // sending the second message
    fprintf(stdout, "client sending message\n");
    strcpy(res.buf, "hello from client\n");
    error = postSend(&res, IBV_WR_SEND_WITH_IMM);
    if (error) {
        fprintf(stderr, "Failed to post send\n");
        return 0;
    }
    error = pollCQ(&res);
    if (error) {
        fprintf(stderr, "Failed to poll cq\n");
        return 0;
    }

    // syncing for buffer ready
    sockSyncData(res.sock, 1, "3", &syncChar);
    // rdma read
    fprintf(stdout, "Client posting rdma read\n");
    error = postSend(&res, IBV_WR_RDMA_READ);
    if (error) {
        fprintf(stdout, "Failed to send rdma read\n");
        return 0;
    }
    error = pollCQ(&res);
    if (error) {
        fprintf(stderr, "Failed to poll cq\n");
        return 0;
    }
    fprintf(stdout, "****\nMessage in server: %s****\n", res.buf);
    
    fprintf(stdout, "client posting rdma write\n");
    strcpy(res.buf, "message in client buffer\n");
    error = postSend(&res, IBV_WR_RDMA_WRITE);
    if (error) {
        fprintf(stdout, "Failed to post rdma write\n");
        return 0;
    }
    error = pollCQ(&res);
    if (error) {
        fprintf(stderr, "Failed to poll cq\n");
        return 0;
    }
    sockSyncData(res.sock, 1, "C", &syncChar);
    return 0;
}