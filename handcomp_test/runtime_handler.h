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


// Method C
extern runtime_props* props;
extern int num_props;

struct runtime_prop {
    size_t stacksize;
    unsigned int deqdepth;
    unsigned int fiber_pool_cap;
    int nworkers;
    int force_reduce;
};

typedef struct runtime_prop runtime_prop;

void populate_runtimes(runtime_prop* props, int num_props, char* var) {
    char *str = getenv(var);
    char *end = str;
    for(int i = 0; i < num_props; i++) {
        int n = strtol(str, &end, 10);
        if (!strcmp(var, "CILK_NWORKERS")) props[i].nworkers = n;
        else if (!strcmp(var, "CILK_STACKSIZE")) props[i].stacksize = n;
        else if (!strcmp(var, "CILK_DEQDEPTH")) props[i].deqdepth = n;
        else if (!strcmp(var, "CILK_FIBER_POOL")) props[i].fiber_pool_cap = n;
        else if (!strcmp(var, "CILK_FORCE_REDUCE")) props[i].force_reduce = n;
        if (*end == ',') {
            end++;
        }
        str = end;
    }
}

struct handle_list {
    global_state** list;
    int capacity;
    int size;
};

typedef struct handle_list handle_list;

struct all_handles {
    handle_list* available;
    handle_list* unavailable;
};

typedef struct all_handles all_handles;

all_handles* runtime_manager;

void runtime_manager_init(int cap) {
    runtime_manager = (all_handles*)malloc(num_props*sizeof(all_handles));
    for (int i = 0; i < num_props; i++) {
        runtime_manager[i]->available = (handle_list*)malloc(sizeof(handle_list));
        runtime_manager[i]->unavailable = (handle_list*)malloc(sizeof(handle_list));
        runtime_manager[i]->available->list = (global_state**)malloc(cap*sizeof(global_state*));
        runtime_manager[i]->unavailable->list = (global_state**)malloc(cap*sizeof(global_state*));
        runtime_manager[i]->available->capacity = cap;
        runtime_manager[i]->unavailable->capacity = cap;
        runtime_manager[i]->available->size = 0;
        runtime_manager[i]->unavailable->size = 0;
    }
}

global_state* get_handle(int prop) {
    handle_list* available = runtime_manager[prop]->available;
    handle_list* unavailable = runtime_manager[prop]->unavailable;

    if (available->size > 0) {
        global_state* res = available[size];
        available->size--;
        unavailable[unavailable->size] = res;
        unavailable->size++;
        return res;
    } else {
        global_state* res = available[size];

    }
}
