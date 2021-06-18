#include <pthreadP.h>
/* We need to assume that there are other threads blocked on the futex.
   See __pthread_mutex_lock_full for further details.  */
#define cilk_pthread_mutex_lock  cilk_pthread_mutex_cond_lock
#define cilk_pthread_mutex_lock_full cilk_pthread_mutex_cond_lock_full

#include <nptl/cilk_pthread_mutex_lock.c>
