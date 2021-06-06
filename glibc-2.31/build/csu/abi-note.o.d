$(common-objpfx)csu/abi-note.o: \
 abi-note.S ../include/stdc-predef.h \
 $(common-objpfx)libc-modules.h \
 ../include/libc-symbols.h \
 $(common-objpfx)config.h \
 ../sysdeps/generic/symbol-hacks.h \
 /usr/lib/gcc/x86_64-linux-gnu/9/include/cet.h \
 $(common-objpfx)csu/abi-tag.h

../include/stdc-predef.h:

$(common-objpfx)libc-modules.h:

../include/libc-symbols.h:

$(common-objpfx)config.h:

../sysdeps/generic/symbol-hacks.h:

/usr/lib/gcc/x86_64-linux-gnu/9/include/cet.h:

$(common-objpfx)csu/abi-tag.h:
