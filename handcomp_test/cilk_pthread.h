
#include <pthread.h>
#include "../runtime/cilk2c.h"
#include "../runtime/cilk2c_inlined.c"

global_state *__cilkrts_startup(int argc, char *argv[]);

struct cilk_pthread_args {
    void *(*func)(void *);
    void *arg;
    int num_workers;
};

typedef struct cilk_pthread_args cilk_pthread_args;

void* cilkify_wrapper(void *arg) {
    cilk_pthread_args* args = (cilk_pthread_args*) arg;
    global_state *cilk_rts = __cilkrts_startup(args->num_workers,NULL);
    __cilkrts_stack_frame sf;
    __enter_cilk_region(cilk_rts, &sf);
    void* res = args->func(args->arg);
    __exit_cilk_region(cilk_rts, &sf);
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
