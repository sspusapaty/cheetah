
#include <pthread.h>
#include <threads.h>
#include "../runtime/cilk2c.h"
#include "../runtime/cilk2c_inlined.c"

global_state *__cilkrts_startup(int argc, char *argv[]);
void __cilkrts_shutdown(global_state *g);
__thread int exit_res;

/*
**thrd_func that dont need changes**
-- thrd_join(thrd_t thr, int *res )
-- thrd_detach(thrd_t thr)
-- thrd_equal(thrd_t lhs, thrd_t rhs)
*/

struct cilk_thrd_args {
    thrd_start_t func;
    void *arg;
    int num_workers;
};

typedef struct cilk_thrd_args cilk_thrd_args;

static int cilkify_wrapper(void *arg) {
    cilk_thrd_args* args = (cilk_thrd_args*) arg;
    global_state *cilk_rts = __cilkrts_startup(args->num_workers,NULL);
    __cilkrts_stack_frame sf;
    __enter_cilk_region(cilk_rts, &sf);
     
    int res;
    __cilkrts_save_fp_ctrl_state(&sf);
    if(!__builtin_setjmp(sf.ctx)) {
        res = args->func(args->arg);
    } else {
        res = exit_res;
        __exit_cilk_region(cilk_rts, &sf);
        __cilkrts_shutdown(cilk_rts);
        free(args);
        thrd_exit(res); // to mirror effects of freeing tls as per spec
    }
    
    __exit_cilk_region(cilk_rts, &sf);
    __cilkrts_shutdown(cilk_rts);
    free(args);
    return res;
}

int cilk_thrd_create(thrd_t *thread, thrd_start_t func, void *arg, int num_workers) {
    cilk_thrd_args* args = (cilk_thrd_args*)malloc(sizeof(cilk_thrd_args));
    args->func = func;
    args->arg = arg;
    args->num_workers = num_workers;
    return thrd_create(thread, cilkify_wrapper, (void*)args);
}

thrd_t cilk_thrd_current() {
    __cilkrts_worker *w = __cilkrts_get_tls_worker();
    if (w == NULL) return thrd_current();
    return w->boss;
}

int thrd_sleep(const struct timespec* duration, struct timespec* remaining) {return -1;} //TODO
void thrd_yield() {} //TODO

// Doesn't work entirely. Either only 1 active worker needs to be guaranteed, or user has put appropriate
// cancellation points
void cilk_thrd_exit(int res) {
    __cilkrts_worker *w = __cilkrts_get_tls_worker();
    
    while(!pthread_mutex_trylock(&(w->g->exit_lock))) { // in case other strands exit at the same time
        pthread_testcancel(); // holding a mutex is not a cancellation point unfortunately
    }
    
    // cancel all other threads
    printf("thread %d is doing exit protocol\n", w->self);
    for (unsigned i = 0; i < w->g->options.nproc; ++i) {
        if (i != w->self){
            printf("cancelling i=%d\n", i);
            int s = pthread_cancel(w->g->threads[i]);
            if (s != 0) printf("error cancelling thread %d\n",i);
        }
    }
    pthread_mutex_unlock(&(w->g->exit_lock));
    
    // find the last stack frame **XXX has memory leak (doesnt destroy closures)**
    __cilkrts_stack_frame *sf = w->current_stack_frame;
    while(!(sf->flags & CILK_FRAME_LAST)) sf = sf->call_parent;
    w->current_stack_frame = sf;
    sf->worker = w;
    
    // store exit code to be retrieved later
    exit_res = res;
    
    // jump back to exit early from cilk region
    sysdep_restore_fp_state(sf);
    __builtin_longjmp(sf->ctx, 1);
}
