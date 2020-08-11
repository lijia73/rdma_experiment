#include "common.hpp"

int main(int argc, char **argv) {
    struct ibv_context  *ctx;
    struct ibv_pd       *pd;
    struct ibv_comp_channel *compChan;
    struct ibv_cq       *cq[2];

    ctx = openDevice();
    if (!ctx) {
        fprintf(stderr, "Failed to open device\n");
        return 0;
    }

    compChan = ibv_create_comp_channel(ctx);
    if (!compChan) {
        fprintf(stderr, "Failed to create completion channel\n");
    }

    cq[0] = ibv_create_cq(ctx, 10, NULL, NULL, 0);
    cq[1] = ibv_create_cq(ctx, 10, NULL, compChan, 0);
    if (!cq[0] || !cq[1]) {
        fprintf(stderr, "Failed to create cq\n");
    }

    ibv_destroy_comp_channel(compChan);
    ibv_destroy_cq(cq[0]);
    ibv_destroy_cq(cq[1]);
}