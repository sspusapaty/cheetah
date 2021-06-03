#pragma once

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <pthread.h>
#include <threads.h>
#include <signal.h>
#include <stdatomic.h>
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

    // TODO: create runtime s.t. all runtime args can be passed in here, not just nworkers
    global_state *cilk_rts = __cilkrts_startup(args->num_workers,NULL);

    __cilkrts_stack_frame sf;
    __enter_cilk_region(cilk_rts, &sf); // cilkify; At this point a worker thread takes control
     
    int res;
    __cilkrts_save_fp_ctrl_state(&sf); // store context here incase a worker needs to return via exit
    if(!__builtin_setjmp(sf.ctx)) {
        res = args->func(args->arg);
    } else { // worker returns via exit
        res = exit_res;
        __exit_cilk_region(cilk_rts, &sf); // uncilkify; orig boss thread takes control
        __cilkrts_shutdown(cilk_rts);
        free(args);
        thrd_exit(res); // to mirror effects of freeing tls as per spec
    }
    
    // Normal return
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

// Can only be called when 1 active worker exists
void cilk_thrd_exit(int res) {
    __cilkrts_worker *w = __cilkrts_get_tls_worker();
    
    // find the last stack frame
    __cilkrts_stack_frame *sf = w->current_stack_frame;
    while(!(sf->flags & CILK_FRAME_LAST)) {
        __cilkrts_pop_frame(sf);
        __cilkrts_leave_frame(sf);
        sf = sf->call_parent;
    }

    w->current_stack_frame = sf;
    sf->worker = w;
    
    // store exit code to be retrieved later
    exit_res = res;
    
    // jump back to exit early from cilk region
    sysdep_restore_fp_state(sf);
    __builtin_longjmp(sf->ctx, 1);
}

// Sets THE exception for all workers, except for the calling worker
// used to trigger other workers to do specific actions (yield or sleep)
void signal_immediate_exception_to_all(__cilkrts_worker *const w) {
    int i, active_size = w->g->nworkers;
    __cilkrts_worker *curr_w;

    for(i=0; i<active_size; i++) {
        if (i == w->self) continue;
        curr_w = w->g->workers[i];
        atomic_store_explicit(&curr_w->exc, EXCEPTION_INFINITY, memory_order_release);
    }
}

// Causes at least one worker thread to yield its timeslice
void cilk_thrd_yield() {
    __cilkrts_worker *w = __cilkrts_get_tls_worker();
   
    // maybe not necessary to lock? What is behavior if two parallel strands call yield/sleep at same time?
    // Is it a data race? Should you linearize the sleeps? Let them race?
    pthread_mutex_lock(&(w->g->exit_lock));
   
    // set YIELD flag to on while current worker yields. Is single global flag the best idea? Perhaps an array of flags?
    atomic_store_explicit(&w->g->thrd_call.type, THRD_YIELD, memory_order_relaxed);
    signal_immediate_exception_to_all(w); // exception reset in the handler

    thrd_yield();

    // reset flag back to NONE to make sure faulty yields don't occur
    atomic_store_explicit(&w->g->thrd_call.type, THRD_NONE, memory_order_relaxed);
    
    pthread_mutex_unlock(&(w->g->exit_lock));
}

// Causes at least one worker thread to sleep for specified duration (doesn't handle signals yet)
// Shares same concerns as yield, along with others
int cilk_thrd_sleep(const struct timespec* time_point, struct timespec* remaining) {
    __cilkrts_worker *w = __cilkrts_get_tls_worker();
   
    pthread_mutex_lock(&(w->g->exit_lock));
    
    // signal exception to all workers
    atomic_store_explicit(&w->g->thrd_call.type, THRD_SLEEP, memory_order_relaxed);
    w->g->thrd_call.time_point = time_point;
    signal_immediate_exception_to_all(w);

    int res = thrd_sleep(time_point, remaining);
   
    w->g->thrd_call.time_point = NULL;
    atomic_store_explicit(&w->g->thrd_call.type, THRD_NONE, memory_order_relaxed);
    
    pthread_mutex_unlock(&(w->g->exit_lock));

    return res;
}

struct cilk_mtx_t {
    atomic_ulong owner; // holds thrd_t
};

typedef struct cilk_mtx_t cilk_mtx_t;

int cilk_mtx_lock(cilk_mtx_t* mtx) {
    thrd_t no_owner = 0;
    while (!atomic_compare_exchange_strong(&(mtx->owner), &no_owner, cilk_thrd_current())){};
    printf("%lu: LOCK mtx->owner = %lu\n", cilk_thrd_current(), mtx->owner);
    return thrd_success;
}

int cilk_mtx_unlock(cilk_mtx_t* mtx) {
    thrd_t curr_owner = cilk_thrd_current();
    if (atomic_compare_exchange_strong(&(mtx->owner), &curr_owner, 0)) return thrd_success;
    printf("%lu: UNLOCK mtx->owner = %lu\n", cilk_thrd_current(), mtx->owner);
    return thrd_error;
}

int cilk_mtx_init(cilk_mtx_t* mtx) {
    atomic_store(&(mtx->owner), 0);
    return thrd_success;
}


