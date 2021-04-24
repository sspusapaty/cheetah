
#include <pthread.h>
#include "../runtime/cilk2c.h"
#include "../runtime/cilk2c_inlined.c"

global_state *__cilkrts_startup(int argc, char *argv[]);
void __cilkrts_shutdown(global_state *g);

struct cilk_pthread_args {
    void *(*func)(void *);
    void *arg;
    int num_workers;
};

typedef struct cilk_pthread_args cilk_pthread_args;

bool cilk_is_worker() {
    return __cilkrts_get_tls_worker() != NULL; 
}

static void* cilkify_wrapper(void *arg) {
    cilk_pthread_args* args = (cilk_pthread_args*) arg;
    global_state *cilk_rts = __cilkrts_startup(args->num_workers,NULL);
    __cilkrts_stack_frame sf;
    __enter_cilk_region(cilk_rts, &sf);
    
    void* res;
    __cilkrts_save_fp_ctrl_state(&sf);
    if(!__builtin_setjmp(sf.ctx)) {
        res = args->func(args->arg);
    }
    
    __exit_cilk_region(cilk_rts, &sf);
    __cilkrts_shutdown(cilk_rts);
    free(args);
    return res;
}

int cilk_pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*func)(void *), void *arg, int num_workers) {
    cilk_pthread_args* args = (cilk_pthread_args*)malloc(sizeof(cilk_pthread_args));
    args->func = func;
    args->arg = arg;
    args->num_workers = num_workers;
    return pthread_create(thread, attr, cilkify_wrapper, (void*)args);
}

pthread_mutex_t m;
void cilk_pthread_exit(void *value_ptr) {
    if (!cilk_is_worker()) {
        pthread_exit(value_ptr);
    } else {
        pthread_mutex_lock(&m);
        __cilkrts_worker *w = __cilkrts_get_tls_worker();
        int s;
        // cancel all other threads
        for (unsigned i = 0; i < w->g->options.nproc; ++i) {
            if (i != w->self){
                printf("cancelling i=%d\n", i);
                s = pthread_cancel(w->g->threads[i]); //SIGKILL
                if (s != 0) printf("error cancelling thread %d\n",i);
            }
        }
        pthread_mutex_unlock(&m);
        
        // find the last stack frame
        __cilkrts_stack_frame *sf = w->current_stack_frame;
        while(!(sf->flags & CILK_FRAME_LAST)) sf = sf->call_parent;
        w->current_stack_frame = sf;
        sf->worker = w;
        
        // jump back to exit early from cilk region
        sysdep_restore_fp_state(sf);
        __builtin_longjmp(sf->ctx, 1);
    }
}
