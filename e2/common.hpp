#include <stdio.h>
#include <netdb.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <byteswap.h>
#include <infiniband/verbs.h>

// #define OPCODE IBV_WR_SEND
#define OPCODE IBV_WR_RDMA_READ

static inline uint64_t htonll(uint64_t x) { return bswap_64(x); }
static inline uint64_t ntohll(uint64_t x) { return bswap_64(x); }

struct connectionData {
    uint64_t addr;
    uint32_t rkey;
    uint32_t qpNum;
    uint16_t lid;
    uint8_t  gid[16];
}__attribute__((packed));

struct resources {
    struct ibv_context  *ctx;
    struct ibv_pd       *pd;
    struct ibv_cq       *cq;
    struct ibv_mr       *mr;
    struct ibv_qp       *qp;
    struct ibv_port_attr portAttr;
    struct connectionData remoteData;
    char                *buf;
    int                 sock;
    int                 immData;
};

struct config_t {
    const char *devName;
    char *serverName;
    u_int32_t tcpPort;
    int ibPort;
    int gidIdx;
};



struct ibv_context *openDevice();
int createResources(struct resources *res);
int createQP(struct resources *res);
int modifyQP2Init(struct resources *res);
int modifyQP2RTR(struct resources *res);
int modifyQP2RTS(struct resources *res);
int modifyQP(struct resources *res);
void destroyResources(struct resources *res);
int connectQP(struct resources *res);
int sockConnect(const char *serverName, int port);
int sockSyncData(int sock, int xferSize, char *localData, char *remoteData);
int postReceive(struct resources *res);
int postSend(struct resources *res, int opcode);
int pollCQ(struct resources *res);