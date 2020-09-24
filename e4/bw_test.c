#include "common.h"
#include <pthread.h>

#define here printf("1\n");

// struct config_t config = {
//     NULL,
//     "10.10.10.2",
//     19875,
//     1,
//     NULL,
//     2
// };

struct config_t config = {
    NULL,
    NULL, 
    19875,
    1,
    NULL, 
    2
};

struct thread_arg {
    struct resources *res;
    int qpn;
};

void * thread_run(void *arg) {
    struct resources *res = ((struct thread_arg*)arg)->res;
    int qpid = ((struct thread_arg*)arg)->qpn;
    unsigned long st;
    unsigned long et;
    unsigned long spent = 0;
    struct timeval cur_time;
    printf("Thread %d created\n", qpid);
    if (config.server_name) {
        for (int i = 0; i < 10; i ++) {
            gettimeofday(&cur_time, NULL);
            st = (cur_time.tv_sec * 1000) + (cur_time.tv_usec / 1000);
            post_send(res, IBV_WR_RDMA_WRITE, qpid);
            poll_completion(res, qpid);
            gettimeofday(&cur_time, NULL);
            et = (cur_time.tv_usec * 1000) + (cur_time.tv_usec / 1000);
            spent += (et - st);
        }
    }
    unsigned long *retval = (unsigned long *)malloc(sizeof(unsigned long));
    *retval = spent;
    return retval;
}

void main() {
    struct resources res;
    char tempChar;
    unsigned long **time_spents;
    pthread_t *ths;
    char syndata;
    
    time_spents = (unsigned long **)malloc(config.num_qp * sizeof(unsigned long *));
    ths = (pthread_t *)malloc(config.num_qp * sizeof(pthread_t));

    resources_init(&res);
    here
    resources_create(&res);
    connect_qp(&res);

    here
    for (int i = 0; i < config.num_qp; i++) {
        int ret;
        struct thread_arg targ = {
            .qpn = i, .res = &res
        };
        ret = pthread_create(&ths[i], NULL, thread_run, &targ);
        if (ret) {
            fprintf(stderr, "Failed to create thread %d\n", i);
            return;
        }
    }
    for (int i = 0; i < config.num_qp; i++) {
        pthread_join(ths[i], (void **)&time_spents[i]);
    }

    for (int i = 0; i < config.num_qp; i++) {
        printf("%d: %lu\n", i, *time_spents[i]);
    }
    sock_sync_data(res.sock, 1, "A", &syndata);
    return;
}