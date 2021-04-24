/*
 * Copyright (c) 1994-2003 Massachusetts Institute of Technology
 * Copyright (c) 2003 Bradley C. Kuszmaul
 * Copyright (c) 2013 I-Ting Angelina Lee and Tao B. Schardl 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#define _GNU_SOURCE
#include <sched.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "cilk_pthread.h"
//#include <cilk/cilk.h>
#include <sys/time.h>

unsigned long long todval (struct timeval *tp) {
    return tp->tv_sec * 1000 * 1000 + tp->tv_usec;
}

void __attribute__((weak)) dummy(void *p) { return; }

static void __attribute__ ((noinline)) fib_spawn_helper(int *x, int n); 

int fib(int n) {
    int x, y, _tmp;

    if(n < 2)
        return n;

    dummy(alloca(0));
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

struct args {
    int val;
};

void* dispatch(void *n) {
    struct args* x = (struct args*)n;
    printf("hello worker? %d\n", cilk_is_worker());
    printf("%d\n", fib(x->val));
    printf("hello worker? %d\n", cilk_is_worker());
    return NULL;
}

#define NUM_TESTS 4
int main(int argc, char** argv) {
    struct timeval t1, t2;

    gettimeofday(&t1,0);

    pthread_t p1, p2;

    struct args a = {42};
    cilk_pthread_create(&p1, NULL, dispatch, (void*)&a, 5);
    //cilk_pthread_create(&p2, NULL, dispatch, (void*)&a, 4);
    pthread_join(p1, NULL);
    //pthread_join(p2, NULL);
    
    gettimeofday(&t2,0);
    
    unsigned long long runtime_ms = (todval(&t2)-todval(&t1))/1000;
    printf("time = %f\n", runtime_ms/1000.0);

    return 0;
}
