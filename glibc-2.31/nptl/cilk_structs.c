#include <stddef.h>
#include "pthreadP.h"
#include "cilk_structs.h"

__cilkrts_worker* (*get_cilk_worker)(void) = NULL;
pid_t (*get_boss_tid)(void) = NULL;

void reg_get_cilk_worker(__cilkrts_worker* (*func)(void)) {
    get_cilk_worker = func;
}

void reg_get_boss_tid(pid_t (*func)(void)) {
    get_boss_tid = func;
}

