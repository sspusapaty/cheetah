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
#define GET_VARIABLE_NAME(Variable) (#Variable)

struct args {
    int val;
};

int dispatch(void *n) {
    struct args* x = (struct args*)n;
    int r = fib(x->val);
    cilk_thrd_exit(thrd_success);
    return r;
}

bool one_thrd_create_join_success(int num_workers) {
    thrd_t thread;
    struct args arg = {42};
    int res = cilk_thrd_create(&thread, dispatch, (void*)&arg, num_workers);
    assert(res == thrd_success);
    
    int answer;
    res = thrd_join(thread, &answer);
    assert(res == thrd_success);
   
    return answer == thrd_success;
}

int main(int argc, char** argv) {

    printf("Running thrd_exit tests...\n");
    run_test("one_thrd_create_join_success", one_thrd_create_join_success, 4);
    return 0;
}
