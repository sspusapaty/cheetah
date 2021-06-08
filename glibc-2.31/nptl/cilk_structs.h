#ifndef CILK_STRUCTS_H_
#define CILK_STRUCTS_H_

struct __cilkrts_stack_frame;
typedef struct __cilkrts_stack_frame __cilkrts_stack_frame;
struct global_state;
typedef struct global_state global_state;
struct local_state;
typedef struct local_state local_state;
struct cilkred_map;
typedef struct cilkred_map cilkred_map;

typedef uint32_t worker_id;
typedef unsigned long int thrd_t;

struct __cilkrts_worker {
    // T and H pointers in the THE protocol
    _Atomic(__cilkrts_stack_frame **) tail;
    _Atomic(__cilkrts_stack_frame **) head;
    _Atomic(__cilkrts_stack_frame **) exc;

    // Limit of the Lazy Task Queue, to detect queue overflow
    __cilkrts_stack_frame **ltq_limit;

    // Worker id, a small integer
    worker_id self;

    // Global state of the runtime system, opaque to the client.
    global_state *g;

    // Additional per-worker state hidden from the client.
    local_state *l;

    // A slot that points to the currently executing Cilk frame.
    __cilkrts_stack_frame *current_stack_frame;

    // Map from reducer names to reducer values
    cilkred_map *reducer_map;

    // Hold info of boss thread
    thrd_t boss;
    pid_t boss_tid;
};

typedef struct __cilkrts_worker __cilkrts_worker;

extern __cilkrts_worker* (*get_cilk_worker)(void);
extern pid_t (*get_boss_tid)(void);

extern void reg_get_cilk_worker(__cilkrts_worker* (*func)(void));
extern void reg_get_boss_tid(pid_t (*func)(void));

#endif
