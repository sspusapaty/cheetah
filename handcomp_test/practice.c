#include <cilk/cilk.h>
#include <pthread.h>


int fib(int x) {
    if (x < 2) return x;
    int y = cilk_spawn fib(x-1);
    int z = fib(x-2);
    cilk_sync;
    return y + z;
}

int fib3(int x) {
    if (x < 2) return x;
    int y = cilk_spawn fib3(x-1);
    int w = cilk_spawn fib3(x-2);
    int z = fib3(x-3);
    cilk_sync;
    return y + z + w;
}

void* f1(void* x) {
    int n = (int)x;
    int res;
    for (int i = 0; i < 10; i++) res = fib(n);
    printf("res = %d\n", res);
}

void* f2(void* x) {
    int n = (int)x;
    int res;
    for (int i = 0; i < 10; i++) res = fib(n);
    printf("3res = %d\n", res);
}

int main() {

    pthread_t t1,t2;
    int n = 42;
    fib(42);
    //pthread_create(&t1, NULL, f1, (void*)n);

    //int m = 42;
    //pthread_create(&t2, NULL, f2, (void*)m);

    //pthread_join(t1, NULL);
    //pthread_join(t2, NULL);
    
    return 0;
}
