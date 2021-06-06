include ../config.mk

SRCS = $(wildcard *.c)
HDRS = $(wildcard *.h)
OBJS = $(patsubst %.c,%.o,$(SRCS))

DEFINES = $(ABI_DEF)

THRD_TESTS = thrd_tests/cilk_thrd_create thrd_tests/cilk_thrd_current_equals thrd_tests/cilk_thrd_sleep_yield thrd_tests/cilk_thrd_exit
TESTS   = cilksort fib mm_dac nqueens $(THRD_TESTS)
OPTIONS = $(OPT) $(ARCH) $(DBG) -Wall $(DEFINES) -fno-omit-frame-pointer
# dynamic linking
# RTS_DLIBS = -L../runtime -Wl,-rpath -Wl,../runtime -lopencilk
# RTS_LIBS = ../runtime/$(RTS_LIB).so
# static linking
RTS_LIBS = ../runtime/$(RTS_LIB).a
TIMING_COUNT := 1
GLIBC_LOC = ../glibc-2.31/build/install/lib
GLIBC = -Wl,--rpath=$(GLIBC_LOC):/usr/lib/x86_64-linux-gnu:/lib/x86_64-linux-gnu -Wl,--dynamic-linker=$(GLIBC_LOC)/ld-linux-x86-64.so.2

.PHONY: all check memcheck clean

all: $(TESTS)

$(TESTS): %: %.o ktiming.o getoptions.o ZERO.o
	$(CC) $(GLIBC) $^ -o $@ $(RTS_LIBS) -lrt -lpthread -lm -lc

%.o: %.c
	$(CC) -c $(OPTIONS) -DTIMING_COUNT=$(TIMING_COUNT) -o $@ $<

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
