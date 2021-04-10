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
#include "cilk_helper.h"
#define MAX_CORES 8

global_state *__cilkrts_startup(int argc, char *argv[]);
const int x = 40;

int stick_this_thread_to_core(int core_id) {
   cpu_set_t cpuset;
   CPU_ZERO(&cpuset);
   CPU_SET(core_id, &cpuset);

   pthread_t current_thread = pthread_self();
   return pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
}

#define ZERO 0
void __attribute__((weak)) dummy(void *p) { return; }

static void __attribute__ ((noinline)) fib_spawn_helper(int *x, int n, __cilkrts_stack_frame *sf1, __cilkrts_stack_frame *sf2); 

int fib(int n, __cilkrts_stack_frame *sf1, __cilkrts_stack_frame *sf2) {
    int x, y, _tmp;

    if(n < 2) {
        return n;
    }
    dummy(alloca(ZERO));
    __cilkrts_stack_frame sf;
    __cilkrts_enter_frame(&sf);

    /* x = spawn fib(n-1) */
    __cilkrts_save_fp_ctrl_state(&sf);
    if(!__builtin_setjmp(sf.ctx)) {
      fib_spawn_helper(&x, n-1, sf1, sf2);
    }
    
    y = fib(n - 2, sf1, sf2);

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

static void __attribute__ ((noinline)) fib_spawn_helper(int *x, int n, __cilkrts_stack_frame *sf1, __cilkrts_stack_frame *sf2) {
    __cilkrts_stack_frame sf;
    __cilkrts_enter_frame_fast(&sf);
    __cilkrts_detach(&sf);
    *x = fib(n, sf1, sf2);
    __cilkrts_pop_frame(&sf);
    __cilkrts_leave_frame(&sf); 
}

struct args {
    global_state* handle;
    int core_affinity;
};

global_state* handles[MAX_CORES]; 


static void __attribute__ ((noinline)) helper(int x) {
    __cilkrts_stack_frame sf;
    __cilkrts_enter_frame_fast(&sf);
    __cilkrts_detach(&sf);
    {
        START_CILK(handles[0], 4);
        int f = fib(x, &sf, &sf);
        printf("answer3 = %d\n", f);
        END_CILK(handles[0])
    }
    __cilkrts_pop_frame(&sf);
    __cilkrts_leave_frame(&sf); 
}

int spawn_funcs(int n) {
    {
        dummy(alloca(ZERO));
        
        START_CILK(handles[1], 0);

        /* x = spawn fib(n-1) */
        __cilkrts_save_fp_ctrl_state(&sf);
        if(!__builtin_setjmp(sf.ctx)) {
          helper(n);
        }
        
        printf("thread1 = %d", fib(n, NULL, NULL));

        /* cilk_sync */
        if(sf.flags & CILK_FRAME_UNSYNCHED) {
          __cilkrts_save_fp_ctrl_state(&sf);
          if(!__builtin_setjmp(sf.ctx)) {
            __cilkrts_sync(&sf);
          }
        }

        END_CILK(handles[1])
    }
    return 0;
}

void print_lock_attempts(__cilkrts_worker *w, void *d) {
    printf("handle = %p, worker id = %d, dlock_a = %lu\n", w->g, w->self, w->dlock_a);
    printf("handle = %p, worker id = %d, dlockself_a = %lu\n", w->g, w->self, w->dlockself_a);
}

#define NUM_TESTS 4
int main(int argc, char** argv) {
    int num_runtimes = 1;
    int cores[] = {0, 4, 2, 6, 1, 3, 7};
    int x = 3;
    char* nested = "single";
    int nworkers[8] = {8, 8, 8, 8, 8, 8, 8, 8};
    if (argc >= 2)
        x = atoi(argv[1]);
    if (argc >= 3)
        nested = argv[2];
    if (argc >= 4)
        nworkers[0] = atoi(argv[3]);
    if (argc >= 5)
        nworkers[1] = atoi(argv[4]);
    if (argc >= 6)
        num_runtimes = atoi(argv[5]);
    for (int i = 0; i < num_runtimes; ++i) {
        handles[i] = __cilkrts_startup(nworkers[i],NULL);
        printf("handles[%d] = %p\n", i, handles[i]);
    }
    printf("%d runtimes made\n", num_runtimes);
    printf("fib(%d) being calculated\n", x);
    printf("%s mode\n", nested);
    for (int i = 0; i < num_runtimes; i++) printf("handle[%d] has %d workers\n", i, handles[i]->nworkers);
    printf("%d runtimes made\n", num_runtimes);
    
    struct timeval t1, t2;

    int f = 0;
    
    if (!strcmp(nested, "nest")) {
        {
            START_CILK(handles[0], 4);
            {
                START_CILK(handles[1], 0);
                gettimeofday(&t1,0);
                f = fib(x, NULL, NULL);
                gettimeofday(&t2,0);
                END_CILK(handles[1])
            }
            END_CILK(handles[0])
        }  
    } else {
        gettimeofday(&t1,0);
        spawn_funcs(42);
        gettimeofday(&t2,0);
        /*
        {
            START_CILK(handles[0], 0);
            gettimeofday(&t1,0);
            f = fib(x, NULL, NULL);
            gettimeofday(&t2,0);
            END_CILK(handles[0])
        }
        */
    }

    //for (int i = 0; i < num_runtimes; i++) for_each_worker(handles[i], print_lock_attempts, NULL);

    printf("answer4 = %d\n",f);

    unsigned long long runtime_ms = (todval(&t2)-todval(&t1))/1000;
    printf("time = %f\n", runtime_ms/1000.0);

    return 0;
}
