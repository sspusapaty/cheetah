
#include <pthread.h>
#include <threads.h>
#include "../runtime/cilk2c.h"
#include "../runtime/cilk2c_inlined.c"

global_state *__cilkrts_startup(int argc, char *argv[]);
void __cilkrts_shutdown(global_state *g);
void signal_immediate_exception_to_all(__cilkrts_worker *const w);
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

void signal_immediate_exception_to_all(__cilkrts_worker *const w) {
    int i, active_size = w->g->nworkers;
    __cilkrts_worker *curr_w;

    for(i=0; i<active_size; i++) {
        curr_w = w->g->workers[i];
        atomic_store_explicit(&curr_w->exc, EXCEPTION_INFINITY, memory_order_release);
    }
}

void reset_exception_to_all(__cilkrts_worker *const w) {
    int i, active_size = w->g->nworkers;
    __cilkrts_worker *curr_w;

    for(i=0; i<active_size; i++) {
        curr_w = w->g->workers[i];
        atomic_store_explicit(&curr_w->exc,
                              atomic_load_explicit(&curr_w->head, memory_order_relaxed),
                              memory_order_release);
    }
}

void cilk_thrd_yield() {
    __cilkrts_worker *w = __cilkrts_get_tls_worker();
   
    printf("about to signal all workers...\n");
    // signal exception to all workers
    
    pthread_mutex_lock(&(w->g->exit_lock));
    
    atomic_store_explicit(&w->g->thrd_call.type, THRD_YIELD, memory_order_relaxed);
    signal_immediate_exception_to_all(w);

    thrd_yield();
    
    atomic_store_explicit(&w->g->thrd_call.type, THRD_NONE, memory_order_relaxed);
    reset_exception_to_all(w);
    
    pthread_mutex_unlock(&(w->g->exit_lock));
}

int cilk_thrd_sleep(const struct timespec* time_point, struct timespec* remaining) {
    __cilkrts_worker *w = __cilkrts_get_tls_worker();
   
    // signal exception to all workers
    pthread_mutex_lock(&(w->g->exit_lock));
    
    atomic_store_explicit(&w->g->thrd_call.type, THRD_SLEEP, memory_order_relaxed);
    w->g->thrd_call.time_point = time_point;
    signal_immediate_exception_to_all(w);

    printf("worker %d sleeping for %ld seconds!\n", w->self, time_point->tv_sec);
    int res = thrd_sleep(time_point, remaining);
   
    w->g->thrd_call.time_point = NULL;
    atomic_store_explicit(&w->g->thrd_call.type, THRD_NONE, memory_order_relaxed);
    //reset_exception_to_all(w);
    
    pthread_mutex_unlock(&(w->g->exit_lock));

    return res;
}
