#include "common.hpp"

void allocBuf(char **buf, int num) {
    for (int i = 0; i < num; i++) {
        buf[i] = (char *)malloc(100);
    }
}

int main(int argc, char * argv[]) {
    struct ibv_context *ctx;
    struct ibv_pd *pd;
    struct ibv_mr *mr[3];
    char *buf[3];

    // open device
    ctx = openDevice();
    if (!ctx) {
        fprintf(stderr, "Failed to open device\n");
    }
    
    // allocate pd
    pd = ibv_alloc_pd(ctx);
    
    // allocate memory buffer
    allocBuf(buf, 3);

    mr[0] = ibv_reg_mr(pd, buf[0], 100, 0);
    mr[1] = ibv_reg_mr(pd, buf[1], 100, IBV_ACCESS_LOCAL_WRITE);
    mr[2] = ibv_reg_mr(pd, buf[2], 100, IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE);
    if (!mr[0] || !mr[1] || !mr[2]) {
        fprintf(stderr, "Failed to register memory\n");
    }

    ibv_dealloc_pd(pd);

    for (int i = 0; i < 3; i++) {
        printf("mr.addr: %x, bufaddr: %x\n", mr[i]->addr, buf[i]);
        ibv_dereg_mr(mr[i]);
        free(buf[i]);
    }
    return 0;
}