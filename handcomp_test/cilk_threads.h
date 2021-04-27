
#include <pthread.h>
#include <threads.h>
#include "../runtime/cilk2c.h"
#include "../runtime/cilk2c_inlined.c"

global_state *__cilkrts_startup(int argc, char *argv[]);
void __cilkrts_shutdown(global_state *g);

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
     
    int res = args->func(args->arg);
    
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

int thrd_sleep(const struct timespec* duration, struct timespec* remaining) {return -1}; //TODO
void thrd_yield() {} //TODO
void thrd_exit(int res) {} //TODO

