#include "common.hpp"

extern config_t config;

struct ibv_context *openDevice() {
    struct ibv_device **deviceList;
    int numDevices;
    deviceList = ibv_get_device_list(&numDevices);

    if (!deviceList) {
        fprintf(stderr, "Failed to get device list\n");
        return NULL;
    }

    struct ibv_context *ctx = ibv_open_device(deviceList[0]);
    ibv_free_device_list(deviceList);
    return ctx;
}

int createQP(struct resources *res) {
    struct ibv_qp_init_attr initAttr;
    memset(&initAttr, 0, sizeof(initAttr));
    initAttr.send_cq = res->cq;
    initAttr.recv_cq = res->cq;
    initAttr.sq_sig_all = 1;
    initAttr.cap.max_send_wr = initAttr.cap.max_recv_wr = 1;
    initAttr.cap.max_send_sge = initAttr.cap.max_recv_sge = 1;
    initAttr.qp_type = IBV_QPT_RC;
    res->qp = ibv_create_qp(res->pd, &initAttr);
    if (!res->qp) {
        return 1;
    }
    return 0;
}

int createResources(struct resources *res) {
    int error = 0;
    int mrFlag;

    res->ctx = openDevice();
    if (!res->ctx) {
        error = 1;
        fprintf(stderr, "Failed to open Device\n");
        goto createResourcesExit;
    }

    error = ibv_query_port(res->ctx, 1, &(res->portAttr));
    if (error) {
        fprintf(stdout, "Failed to query port\n");
        error = 1;
        goto createResourcesExit;
    }
    
    res->buf = (char*)malloc(100);
    if (!res->buf) {
        error = 1;
        goto createResourcesExit;
    }
    
    res->pd = ibv_alloc_pd(res->ctx);
    if (!res->pd) {
        error = 1;
        fprintf(stderr, "Failed to allocate pd\n");
        goto createResourcesExit;
    }
    
    mrFlag = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;
    res->mr = ibv_reg_mr(res->pd, res->buf, 100, mrFlag);
    if (!res->mr) {
        error = 1;
        fprintf(stderr, "Failed to register mr\n");
        goto createResourcesExit;
    }

    res->cq = ibv_create_cq(res->ctx, 10, NULL, NULL, 0);
    if (!res->cq) {
        error = 1;
        fprintf(stderr, "Failed to create cq\n");
        goto createResourcesExit;
    }

    error = createQP(res);
    if (error) {
        fprintf(stderr, "Failed to createQP\n");
        goto createResourcesExit;
    }

    res->sock = sockConnect(config.serverName, config.tcpPort);
    if (res->sock < 0) {
        fprintf(stderr, "Failed to connect socket\n");
        goto createResourcesExit;
    }

createResourcesExit:
    if (error) {
        destroyResources(res);
    }
    return error;
}

int modifyQP(struct resources *res) {
    int error = 0;
    error = modifyQP2Init(res);
    if (error) {
        fprintf(stderr, "Failed to modify qp to init\n");
        return error;
    }
    error = modifyQP2RTR(res);
    if (error) {
        fprintf(stderr, "Failed to modify qp to rtr\n");
        return error;
    }
    error = modifyQP2RTS(res);
    if (error) {
        fprintf(stderr, "Failed to modify qp to rts\n");
        return error;
    }
    return error;
}

void destroyResources(struct resources *res) {
    if (res->qp) {
        ibv_destroy_qp(res->qp);
    }
    if (res->cq) {
        ibv_destroy_cq(res->cq);
    }
    if (res->mr) {
        ibv_dereg_mr(res->mr);
    }
    if (res->buf) {
        free(res->buf);
    }
    if (res->pd) {
        ibv_dealloc_pd(res->pd);
    }
    if (res->ctx) {
        ibv_close_device(res->ctx);
    }
    return;
}

int connectQP(struct resources *res) {
    struct connectionData localData;
    struct connectionData remoteData;
    struct connectionData tempData;
    int error = 0;
    char tempChar;
    union ibv_gid mygid;

    memset(&mygid, 0, sizeof(mygid));
    localData.addr = htonll((uintptr_t)res->buf);
    localData.rkey = htonl(res->mr->rkey);
    localData.qpNum = htonl(res->qp->qp_num);
    localData.lid = htons(res->portAttr.lid);
    memcpy(localData.gid, &mygid, 16);
    if (sockSyncData(res->sock, sizeof(struct connectionData), (char*)&localData, (char*)&tempData) < 0) {
        fprintf(stderr, "Failed to sync connection info\n");
        error = 1;
        goto connectQPExit;
    }
    remoteData.addr = ntohll(tempData.addr);
    remoteData.rkey = ntohl(tempData.rkey);
    remoteData.lid = ntohs(tempData.lid);
    remoteData.qpNum = ntohl(tempData.qpNum);
    memcpy(remoteData.gid, tempData.gid, 16);
    res->remoteData = remoteData;

    error = modifyQP2Init(res);
    if (error) {
        fprintf(stderr, "Failed to modify qp to init\n");
        goto connectQPExit;
    } 
    error = modifyQP2RTR(res);
    if (error) {
        fprintf(stderr, "Failed to modify qp to rtr\n");
        goto connectQPExit;
    }
    // if (config.serverName) {
    //     fprintf(stdout, "client posting receive\n");
    //     error = postReceive(res);
    //     if (error) {
    //         fprintf(stderr, "Failed to post receive\n");
    //         goto connectQPExit;
    //     }
    // }
    error = modifyQP2RTS(res);
    if (error) {
        fprintf(stderr, "Failed to modify qp to rts\n");
        goto connectQPExit;
    }
connectQPExit:
    return error;
}

int sockConnect(const char *serverName, int port) {
    struct addrinfo *resolvedAddr = NULL;
    struct addrinfo *iterator;
    char service[6];
    int sockfd = -1;
    int listenfd = 0;
    int tmp;
    struct addrinfo hints = {
        .ai_flags = AI_PASSIVE,
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM
    };
    if (sprintf(service, "%d", port) < 0) 
        goto sockConnectExit;
    sockfd = getaddrinfo(serverName, service, &hints, &resolvedAddr);
    if (sockfd < 0) {
        fprintf(stderr, "%s for %s:%d\n", gai_strerror(sockfd), serverName, port);
        goto sockConnectExit;
    }
    for (iterator = resolvedAddr; iterator; iterator = iterator->ai_next) {
        sockfd = socket(iterator->ai_family, iterator->ai_socktype, iterator->ai_protocol);
        if (sockfd >= 0) {
            if (serverName) {
                if ((tmp = connect(sockfd, iterator->ai_addr, iterator->ai_addrlen))) {
                    fprintf(stderr, "Failed to connect\n");
                    close(sockfd);
                    sockfd = -1;
                }
            } else {
                listenfd = sockfd;
                sockfd = -1;
                if (bind(listenfd, iterator->ai_addr, iterator->ai_addrlen))
                    goto sockConnectExit;
                listen(listenfd, 1);
                sockfd = accept(listenfd, NULL, 0);
            }
        }
    }
sockConnectExit:
    if (listenfd)
        close(listenfd);
    if (resolvedAddr) 
        freeaddrinfo(resolvedAddr);
    if (sockfd < 0) {
        if (serverName) 
            fprintf(stderr, "Counldn't connect to %s:%d", config.serverName, config.tcpPort);
        else {
            perror("server acept");
            fprintf(stderr, "accept() failed\n");
        }
    }
    return sockfd;
}

int sockSyncData(int sock, int xferSize, char *localData, char *remoteData) {
    int error;
    int readBytes = 0;
    int totalReadBytes = 0;
    error = write(sock, localData, xferSize);
    if (error < xferSize) {
        fprintf(stderr, "Failed to send data through tcp\n");
    } else {
        error = 0;
    }
    while(!error && totalReadBytes < xferSize) {
        readBytes = read(sock, remoteData, xferSize);
        if (readBytes > 0)
            totalReadBytes += readBytes;
        else
            error = readBytes;
    }
    return error;
}

int modifyQP2Init(struct resources *res) {
    struct ibv_qp_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.qp_state = IBV_QPS_INIT;
    attr.port_num = config.ibPort;
    attr.pkey_index = 0;
    attr.qp_access_flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ;
    int flags = IBV_QP_STATE | IBV_QP_PKEY_INDEX |
                IBV_QP_ACCESS_FLAGS | IBV_QP_PORT;
    int error = ibv_modify_qp(res->qp, &attr, flags);
    return error;
}

int modifyQP2RTR(struct resources *res) {
    struct ibv_qp_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.qp_state = IBV_QPS_RTR;
    attr.path_mtu = IBV_MTU_256;
    attr.dest_qp_num = res->remoteData.qpNum;
    attr.rq_psn = 0;
    attr.max_dest_rd_atomic = 1;
    attr.min_rnr_timer = 0x12;
    attr.ah_attr.is_global = 0;
    attr.ah_attr.dlid = res->remoteData.lid;
    attr.ah_attr.sl = 0;
    attr.ah_attr.src_path_bits = 0;
    attr.ah_attr.port_num = config.ibPort;
    int flags = IBV_QP_STATE | IBV_QP_AV | IBV_QP_DEST_QPN | 
                IBV_QP_MAX_DEST_RD_ATOMIC | IBV_QP_MIN_RNR_TIMER |
                IBV_QP_RQ_PSN | IBV_QP_PATH_MTU;
    int error = ibv_modify_qp(res->qp, &attr, flags);

    return error;              
}

int modifyQP2RTS(struct resources *res) {
    struct ibv_qp_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.qp_state = IBV_QPS_RTS;
    attr.timeout = 0x12;
    attr.retry_cnt = 6;
    attr.rnr_retry = 0;
    attr.sq_psn = 0;
    attr.max_rd_atomic = 1;
    int flags = IBV_QP_STATE | IBV_QP_TIMEOUT | IBV_QP_RETRY_CNT |
                IBV_QP_RNR_RETRY | IBV_QP_SQ_PSN | IBV_QP_MAX_QP_RD_ATOMIC;
    int error = ibv_modify_qp(res->qp, &attr, flags);
    return error;
}

int postReceive(struct resources *res) {
    struct ibv_recv_wr rr;
    struct ibv_sge sge;
    struct ibv_recv_wr *badrr;
    int error;
    sge.addr = (uintptr_t)res->buf;
    sge.length = 100;
    sge.lkey = res->mr->lkey;
    memset(&rr, 0, sizeof(rr));
    rr.next = NULL;
    rr.num_sge = 1;
    rr.sg_list = &sge;
    rr.wr_id = 0;
    error = ibv_post_recv(res->qp, &rr, &badrr);
    return error;
}

int postSend(struct resources *res, int opcode) {
    struct ibv_send_wr sr;
    struct ibv_send_wr *badsr;
    struct ibv_sge sge = {
        .addr = (uintptr_t)res->buf,
        .length = 100,
        .lkey = res->mr->lkey
    };
    memset(&sr, 0, sizeof(sr));
    sr.opcode = (ibv_wr_opcode)opcode;
    sr.next = NULL;
    sr.wr_id = 0;
    sr.num_sge = 1;
    sr.sg_list = &sge;
    sr.send_flags = IBV_SEND_SIGNALED;
    if (opcode  == IBV_WR_SEND_WITH_IMM) {
        fprintf(stdout, "posting imm\n");
        sr.imm_data = htonl(100);
    }
    if (opcode == IBV_WR_RDMA_READ || opcode == IBV_WR_RDMA_WRITE || 
        opcode == IBV_WR_RDMA_WRITE_WITH_IMM) {
        sr.wr.rdma.remote_addr = res->remoteData.addr;
        sr.wr.rdma.rkey = res->remoteData.rkey;
    }
    int error = ibv_post_send(res->qp, &sr, &badsr);
    return error;
}

int pollCQ(struct resources *res) {
    struct ibv_wc wc;
    int numComp;
    int error = 0;
    
    do {
        numComp = ibv_poll_cq(res->cq, 1, &wc);
    } while(numComp == 0);

    if (numComp < 0) {
        error = 1;
        return error;
    }
    fprintf(stdout, "CQ opcode: %d\n", wc.opcode);
    if (wc.status != IBV_WC_SUCCESS) {
        fprintf(stderr, "Failed status %s (%d) for wr_id %d\n", 
                ibv_wc_status_str(wc.status), wc.status, (int)wc.wr_id);
        error = 1;
        return error;
    }
    if (wc.wc_flags | IBV_WC_WITH_IMM) {
        fprintf(stdout, "wc with imm\n");
        res->immData = ntohl(wc.imm_data);
    }
    return error;
}