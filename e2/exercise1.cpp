#include <stdio.h>
#include <infiniband/verbs.h>

int main(int argc, int *argv[]) {
    struct ibv_device **deviceList = NULL;
    int numDevice;

    deviceList = ibv_get_device_list(&numDevice);
    if (deviceList == NULL) {
        fprintf(stderr, "failed to find ibv devices\n");
        return -1;
    }

    for (int i = 0; i < numDevice; i++) {
        printf("RDMA device[%d]: name=%s, type=%s\n", i, 
               ibv_get_device_name(deviceList[i]),
               ibv_node_type_str(deviceList[i]->node_type));
        struct ibv_context *ctx = ibv_open_device(deviceList[i]);
        struct ibv_port_attr portAttr;
        int rc = ibv_query_port(ctx, 1, &portAttr);
        if (rc) {
            fprintf(stderr, "Failed to query port %d attributes in device %s\n",
                    1, ibv_get_device_name(ctx->device));
        }
        union ibv_gid gid;
        rc = ibv_query_gid(ctx, 1, 5, &gid);
        if (rc) {
            fprintf(stderr, "Failed to query gid\n");
        }
        printf("gid: %x %x", gid.global.subnet_prefix, gid.global.interface_id);
    }
    ibv_free_device_list(deviceList);
    return 0;
}