#include "common.hpp"

struct config_t config;

int main(int argc, char **argv) {
    config.devName = NULL;
    config.serverName = NULL;
    config.tcpPort = 19971;
    config.ibPort = 1;
    config.gidIdx = -1;

    char syncChar;

    struct resources res;
    res.immData = 0;
    fprintf(stdout, "server creating resources\n");
    int error = createResources(&res);
    strcpy(res.buf, "hello from server\n");
    if (error) {
        fprintf(stderr, "Failed to create resources\n");
        return 0;
    }

    fprintf(stdout, "server connecting qp\n");
    error = connectQP(&res);
    if (error) {
        fprintf(stderr, "Failed to connect qp\n");
        return 0;
    }

    // sync after connect qp
    error = sockSyncData(res.sock, 1, "1", &syncChar);
    if (syncChar != '1') {
        return 0;
    }
    if (error) {
        fprintf(stderr, "Failed to sync after connecting qp\n");
        return 0;
    }

    // sending the first message
    fprintf(stdout, "server sending message\n");
    error = postSend(&res, IBV_WR_SEND_WITH_IMM);
    if (error) {
        fprintf(stderr, "Failed to post send\n");
        return 0;
    }

    fprintf(stdout, "server polling cq\n");
    error = pollCQ(&res);
    if (error) {
        fprintf(stdout, "failed to poll cq\n");
        return 0;
    }
    fprintf(stdout, "Message sent successfully\n");

    // posting the second receive
    fprintf(stdout, "server posting receive\n");
    error = postReceive(&res);
    if (error) {
        fprintf(stderr, "Failed to post receive\n");
        return 0;
    }
    // syncing for receive ready
    sockSyncData(res.sock, 1, "2", &syncChar);
    if (syncChar != '2') {
        return 0;
    }

    // polling for receiving 
    fprintf(stdout, "server polling cq\n");
    error = pollCQ(&res);
    if (error) {
        fprintf(stderr, "Failed to poll cq\n");
        return 0;
    }
    fprintf(stdout, "****\nMessage received: %s\nImm data: %d\n****\n", res.buf, res.immData);
    

    // preparing buffer for rdma read
    strcpy(res.buf, "message in server buffer\n");
    // sync buffer ready
    sockSyncData(res.sock, 1, "3", &syncChar);
    // wait for rdma read and write
    sockSyncData(res.sock, 1, "S", &syncChar);
    
    fprintf(stdout, "Message in buffer now: %s\n", res.buf);

    return 0;
}