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
#include "../cilk_threads.h"
#include "util.h"
#include <sys/time.h>

struct args {
    int val;
    int flag; // 0 is sleep, 1 is fib, 2 is yield
    int flag2;
};

static void __attribute__ ((noinline)) dispatch_helper(int flag2) {
    __cilkrts_stack_frame sf;
    __cilkrts_enter_frame_fast(&sf);
    __cilkrts_detach(&sf);
    if (flag2 == 0) cilk_thrd_sleep(&(struct timespec){.tv_sec=10}, NULL);
    else if (flag2 == 2) cilk_thrd_yield();
    __cilkrts_pop_frame(&sf);
    __cilkrts_leave_frame(&sf); 
}

int dispatch(void *n) {
    struct args* x = (struct args*)n;
    
    dummy(alloca(0));
    __cilkrts_stack_frame sf;
    __cilkrts_enter_frame(&sf);
    
    __cilkrts_save_fp_ctrl_state(&sf);
    if(!__builtin_setjmp(sf.ctx)) {
      dispatch_helper(x->flag2);
    }
    
    int r = 0;
    if (x->flag == 0) r = cilk_thrd_sleep(&(struct timespec){.tv_sec=7}, NULL);
    else if (x->flag == 1) r = fib(x->val); // approximately 7 seconds
    else cilk_thrd_yield();
    
    if(sf.flags & CILK_FRAME_UNSYNCHED) {
      __cilkrts_save_fp_ctrl_state(&sf);
      if(!__builtin_setjmp(sf.ctx)) {
        __cilkrts_sync(&sf);
      }
    }
    
    __cilkrts_pop_frame(&sf);
    if (0 != sf.flags)
        __cilkrts_leave_frame(&sf);
    
    return r;
}

// Sleep and fib in parallel
bool thrd_sleep_fib(int num_workers) {
    thrd_t thread;
    struct args arg = {42,1,0}; // should take 7 + 10 seconds to run
    int res = cilk_thrd_create(&thread, dispatch, (void*)&arg, num_workers);
    assert(res == thrd_success);
    
    int answer;
    res = thrd_join(thread, &answer);
    assert(res == thrd_success);
   
    return answer == 267914296;
}

bool thrd_yield_fib(int num_workers) {
    thrd_t thread;
    struct args arg = {42,1,2}; // should take more than 7 seconds
    int res = cilk_thrd_create(&thread, dispatch, (void*)&arg, num_workers);
    assert(res == thrd_success);
    
    int answer;
    res = thrd_join(thread, &answer);
    assert(res == thrd_success);
   
    return answer == 267914296;
}

// Two cilk_sleeps in parallel (what is behavior?)
bool thrd_sleep_sleep(int num_workers) {
    thrd_t thread;
    struct args arg = {42,0,0}; // should take 7 + 10 seconds to run
    int res = cilk_thrd_create(&thread, dispatch, (void*)&arg, num_workers);
    assert(res == thrd_success);
    
    int answer;
    res = thrd_join(thread, &answer);
    assert(res == thrd_success);
   
    return answer == 0;
}

int main(int argc, char** argv) {

    printf("Running thrd_sleep tests...\n");
    run_test("thrd_sleep_fib", thrd_sleep_fib, 4);
    run_test("thrd_sleep_sleep", thrd_sleep_sleep, 4);
    run_test("thrd_yield_fib", thrd_yield_fib, 4);

    return 0;
}
