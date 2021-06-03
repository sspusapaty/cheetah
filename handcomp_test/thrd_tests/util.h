#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "../cilk_threads.h"
#include "../cilk_mutex.h"
#define ZERO 0

unsigned long long todval (struct timeval *tp) {
    return tp->tv_sec * 1000 * 1000 + tp->tv_usec;
}

void run_test(char* name, bool (*func)(int), int arg) {
    struct timeval t1, t2;
    gettimeofday(&t1,0);
    bool res = func(arg);
    gettimeofday(&t2,0);
    unsigned long long runtime_ms = (todval(&t2)-todval(&t1))/1000;
    
    char* pass = "PASS";
    if (!res) pass = "FAIL";
    
    printf("TEST: %s(%d) = %s -- %f\n", name, arg, pass, runtime_ms/1000.0);
}

void __attribute__((weak)) dummy(void *p) { return; }

static void __attribute__ ((noinline)) fib_spawn_helper(int *x, int n); 

int fib(int n) {
    int x, y, _tmp;

    if(n < 2)
        return n;

    dummy(alloca(ZERO));
    __cilkrts_stack_frame sf;
    __cilkrts_enter_frame(&sf);

    /* x = spawn fib(n-1) */
    __cilkrts_save_fp_ctrl_state(&sf);
    if(!__builtin_setjmp(sf.ctx)) {
      fib_spawn_helper(&x, n-1);
    }

    y = fib(n - 2);

    /* cilk_sync */
    if(sf.flags & CILK_FRAME_UNSYNCHED) {
      __cilkrts_save_fp_ctrl_state(&sf);
      if(!__builtin_setjmp(sf.ctx)) {
        __cilkrts_sync(&sf);
      }
    }
    _tmp = x + y;

    __cilkrts_pop_frame(&sf);
    if (0 != sf.flags)
        __cilkrts_leave_frame(&sf);

    return _tmp;
}

static void __attribute__ ((noinline)) fib_spawn_helper(int *x, int n) {

    __cilkrts_stack_frame sf;
    __cilkrts_enter_frame_fast(&sf);
    __cilkrts_detach(&sf);
    *x = fib(n);
    __cilkrts_pop_frame(&sf);
    __cilkrts_leave_frame(&sf); 
}
