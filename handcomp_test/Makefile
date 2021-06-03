include ../config.mk

SRCS = $(wildcard *.c)
HDRS = $(wildcard *.h)
OBJS = $(patsubst %.c,%.o,$(SRCS))

DEFINES = $(ABI_DEF)

MUTEX_REQS = 
THRD_TESTS = $(MUTEX_REQS) thrd_tests/cilk_thrd_create thrd_tests/cilk_thrd_current_equals thrd_tests/cilk_thrd_sleep_yield thrd_tests/cilk_thrd_exit thrd_tests/cilk_mtx
TESTS   = $(THRD_TESTS)
OPTIONS = $(OPT) $(ARCH) $(DBG) -Wall $(DEFINES) -fno-omit-frame-pointer
# dynamic linking
# RTS_DLIBS = -L../runtime -Wl,-rpath -Wl,../runtime -lopencilk
# RTS_LIBS = ../runtime/$(RTS_LIB).so
# static linking
RTS_LIBS = ../runtime/$(RTS_LIB).a
TIMING_COUNT := 1


.PHONY: all check memcheck clean

all: $(TESTS)

$(TESTS): %: %.o cilk_mutex_reqs/dl-tunables.o cilk_mutex_reqs/lowlevellock.o  cilk_mutex_reqs/pthread_mutex_conf.o  cilk_mutex_reqs/tpp.o  cilk_mutex_reqs/vars.o cilk_mutex_reqs/_exit.o  cilk_mutex_reqs/dl-misc.o cilk_mutex_reqs/sbrk.o cilk_mutex_reqs/assert.o#ktiming.o getoptions.o ZERO.o
	$(CC) $^ -o $@ $(RTS_LIBS) -lrt -lpthread -lm

%.o: %.c
	$(CC) -c $(OPTIONS) -DTIMING_COUNT=$(TIMING_COUNT) -I/tmp/glibc-2.31/include -I/tmp/build/nptl  -I/tmp/build  -I/tmp/glibc-2.31/sysdeps/unix/sysv/linux/x86_64/64  -I/tmp/glibc-2.31/sysdeps/unix/sysv/linux/x86_64  -I/tmp/glibc-2.31/sysdeps/unix/sysv/linux/x86/include -I/tmp/glibc-2.31/sysdeps/unix/sysv/linux/x86  -I/tmp/glibc-2.31/sysdeps/x86/nptl  -I/tmp/glibc-2.31/sysdeps/unix/sysv/linux/wordsize-64  -I/tmp/glibc-2.31/sysdeps/x86_64/nptl  -I/tmp/glibc-2.31/sysdeps/unix/sysv/linux/include -I/tmp/glibc-2.31/sysdeps/unix/sysv/linux  -I/tmp/glibc-2.31/sysdeps/nptl  -I/tmp/glibc-2.31/sysdeps/pthread  -I/tmp/glibc-2.31/sysdeps/gnu  -I/tmp/glibc-2.31/sysdeps/unix/inet  -I/tmp/glibc-2.31/sysdeps/unix/sysv  -I/tmp/glibc-2.31/sysdeps/unix/x86_64  -I/tmp/glibc-2.31/sysdeps/unix  -I/tmp/glibc-2.31/sysdeps/posix  -I/tmp/glibc-2.31/sysdeps/x86_64/64  -I/tmp/glibc-2.31/sysdeps/x86_64/fpu/multiarch  -I/tmp/glibc-2.31/sysdeps/x86_64/fpu  -I/tmp/glibc-2.31/sysdeps/x86/fpu/include -I/tmp/glibc-2.31/sysdeps/x86/fpu  -I/tmp/glibc-2.31/sysdeps/x86_64/multiarch  -I/tmp/glibc-2.31/sysdeps/x86_64  -I/tmp/glibc-2.31/sysdeps/x86  -I/tmp/glibc-2.31/sysdeps/ieee754/float128  -I/tmp/glibc-2.31/sysdeps/ieee754/ldbl-96/include -I/tmp/glibc-2.31/sysdeps/ieee754/ldbl-96  -I/tmp/glibc-2.31/sysdeps/ieee754/dbl-64/wordsize-64  -I/tmp/glibc-2.31/sysdeps/ieee754/dbl-64  -I/tmp/glibc-2.31/sysdeps/ieee754/flt-32  -I/tmp/glibc-2.31/sysdeps/wordsize-64  -I/tmp/glibc-2.31/sysdeps/ieee754  -I/tmp/glibc-2.31/sysdeps/generic  -I/tmp/glibc-2.31 -I/tmp/glibc-2.31/libio -I. -D_LIBC_REENTRANT -include /tmp/build/libc-modules.h -DMODULE_NAME=libpthread -include /tmp/glibc-2.31/include/libc-symbols.h -o $@ $<

memcheck: 
	$(MAKE) clean; $(MAKE) > /dev/null
	date
	CILK_NWORKERS=8 valgrind ./fib 26
	CILK_NWORKERS=8 valgrind ./mm_dac -n 512
	CILK_NWORKERS=8 valgrind ./cilksort -n 3000000
	CILK_NWORKERS=8 valgrind ./nqueens 10
	date

check:
	$(MAKE) clean; $(MAKE) TIMING_COUNT=5 > /dev/null
	CILK_NWORKERS=$(MANYPROC) ./fib 40
	CILK_NWORKERS=$(MANYPROC) ./mm_dac -n 1024 -c
	CILK_NWORKERS=$(MANYPROC) ./cilksort -n 30000000 -c
	CILK_NWORKERS=$(MANYPROC) ./nqueens 14

clean:
	rm -f thrd_tests/*.o *.o *~ $(TESTS) core.*
