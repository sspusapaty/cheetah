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

#include <sys/time.h>
#include <pthread.h>
#include "../runtime/cilk2c.h"
#include "../runtime/cilk2c_inlined.c"

// Method C

#define START_CILK(handle, core) \
    __cilkrts_stack_frame sf; \
    __enter_cilk_region(handle, &sf, core);

#define END_CILK(handle) __exit_cilk_region(handle, &sf);

#define CILK_REGION(handle,core,code) { \
                            START_CILK(handle, core) \
                            code \
                            END_CILK(handle) }

// Method A
#define CILKIFY(h) __cilkrts_stack_frame sf; \
                   cilkify(h, &sf);

#define UNCILKIFY(h) uncilkify(h, &sf);

// Method B
#define RUN_ON_CILK(h, r, func, ...) __cilkrts_stack_frame sf; \
                                  cilkify(h, &sf); \
                                  *r = func(__VA_ARGS__); \
                                  uncilkify(h, &sf);

unsigned long long todval (struct timeval *tp) {
    return tp->tv_sec * 1000 * 1000 + tp->tv_usec;
}

