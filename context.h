typedef struct {
    LIST *ctx_recvlist;
    LIST *ctx_sendlist;
    
    pthread_t *thread_in;
    pthread_t *thread_out;
    pthread_t *thread_recv;
    pthread_t *thread_send;

    pthread_mutex_t mut_send;
    pthread_mutex_t mut_recv;
    pthread_mutex_t mut_sendcv;
    pthread_mutex_t mut_recvcv;
    pthread_mutex_t mut_render;
    
    pthread_cond_t cv_send;
    pthread_cond_t cv_recv;

    char *ip;
    int ctx_confd;

    struct notcurses *ctx_nc;
    struct ncplane *plane_log;
    struct ncplane *plane_prompt;
    int ctx_rows;
    int ctx_cols;
    char *ctx_closed;
  } CONTEXT;
