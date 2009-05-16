#
# Tiny C Compiler Makefile
#

TOP ?= .
include $(TOP)/config.mak

CFLAGS+=-g -Wall
CFLAGS_P=$(CFLAGS) -pg -static -DCONFIG_TCC_STATIC
LIBS_P=

ifneq ($(GCC_MAJOR),2)
CFLAGS+=-fno-strict-aliasing
endif

ifeq ($(ARCH),i386)
CFLAGS+=-mpreferred-stack-boundary=2
ifeq ($(GCC_MAJOR),2)
CFLAGS+=-m386 -malign-functions=0
else
CFLAGS+=-march=i386 -falign-functions=0
ifneq ($(GCC_MAJOR),3)
CFLAGS+=-Wno-pointer-sign -Wno-sign-compare -D_FORTIFY_SOURCE=0
endif
endif
endif

ifeq ($(ARCH),x86-64)
CFLAGS+=-Wno-pointer-sign
endif

ifndef CONFIG_WIN32
LIBS=-lm
ifndef CONFIG_NOLDL
LIBS+=-ldl
endif
endif

ifdef CONFIG_WIN32
NATIVE_TARGET=-DTCC_TARGET_PE
LIBTCC1=libtcc1.a
else
ifeq ($(ARCH),i386)
NATIVE_TARGET=-DTCC_TARGET_I386
LIBTCC1=libtcc1.a
BCHECK_O=bcheck.o
else
ifeq ($(ARCH),arm)
NATIVE_TARGET=-DTCC_TARGET_ARM
NATIVE_TARGET+=$(if $(wildcard /lib/ld-linux.so.3),-DTCC_ARM_EABI)
NATIVE_TARGET+=$(if $(shell grep -l "^Features.* \(vfp\|iwmmxt\) " /proc/cpuinfo),-DTCC_ARM_VFP)
else
ifeq ($(ARCH),x86-64)
NATIVE_TARGET=-DTCC_TARGET_X86_64
LIBTCC1=libtcc1.a
endif
endif
endif
endif

ifneq ($(wildcard /lib/ld-uClibc.so.0),)
NATIVE_TARGET+=-DTCC_UCLIBC
BCHECK_O=
endif

ifdef CONFIG_USE_LIBGCC
LIBTCC1=
endif

ifeq ($(TOP),.)

PROGS=tcc$(EXESUF)

I386_CROSS = i386-tcc$(EXESUF)
WIN32_CROSS = i386-win32-tcc$(EXESUF)
X64_CROSS = x86_64-tcc$(EXESUF)
ARM_CROSS = arm-tcc-fpa$(EXESUF) arm-tcc-fpa-ld$(EXESUF) \
    arm-tcc-vfp$(EXESUF) arm-tcc-vfp-eabi$(EXESUF)
C67_CROSS = c67-tcc$(EXESUF)

CORE_FILES = tcc.c libtcc.c tccpp.c tccgen.c tccelf.c tccasm.c \
    tcc.h config.h libtcc.h tcctok.h
I386_FILES = $(CORE_FILES) i386-gen.c i386-asm.c i386-asm.h
WIN32_FILES = $(CORE_FILES) i386-gen.c i386-asm.c i386-asm.h tccpe.c
X86_64_FILES = $(CORE_FILES) x86_64-gen.c
ARM_FILES = $(CORE_FILES) arm-gen.c
C67_FILES = $(CORE_FILES) c67-gen.c tcccoff.c

ifdef CONFIG_WIN32
PROGS+=tiny_impdef$(EXESUF) tiny_libmaker$(EXESUF)
NATIVE_FILES=$(WIN32_FILES)
PROGS_CROSS=$(I386_CROSS) $(X64_CROSS) $(ARM_CROSS) $(C67_CROSS)
else
ifeq ($(ARCH),i386)
NATIVE_FILES=$(I386_FILES)
PROGS_CROSS=$(X64_CROSS) $(WIN32_CROSS) $(ARM_CROSS) $(C67_CROSS)
else
ifeq ($(ARCH),x86-64)
NATIVE_FILES=$(X86_64_FILES)
PROGS_CROSS=$(I386_CROSS) $(WIN32_CROSS) $(ARM_CROSS) $(C67_CROSS)
else
ifeq ($(ARCH),arm)
NATIVE_FILES=$(ARM_FILES)
PROGS_CROSS=$(I386_CROSS) $(X64_CROSS) $(WIN32_CROSS) $(C67_CROSS)
endif
endif
endif
endif

ifdef CONFIG_CROSS
PROGS+=$(PROGS_CROSS)
endif

all: $(PROGS) $(LIBTCC1) $(BCHECK_O) libtcc.a tcc-doc.html tcc.1 libtcc_test$(EXESUF)

# Host Tiny C Compiler
tcc$(EXESUF): $(NATIVE_FILES)
	$(CC) -o $@ $< $(NATIVE_TARGET) $(CFLAGS) $(LIBS)

# Cross Tiny C Compilers
i386-tcc$(EXESUF): $(I386_FILES)
	$(CC) -o $@ $< -DTCC_TARGET_I386 $(CFLAGS) $(LIBS)

i386-win32-tcc$(EXESUF): $(WIN32_FILES)
	$(CC) -o $@ $< -DTCC_TARGET_PE $(CFLAGS) $(LIBS)

x86_64-tcc$(EXESUF): $(X86_64_FILES)
	$(CC) -o $@ $< -DTCC_TARGET_X86_64 $(CFLAGS) $(LIBS)

c67-tcc$(EXESUF): $(C67_FILES)
	$(CC) -o $@ $< -DTCC_TARGET_C67 $(CFLAGS) $(LIBS)

arm-tcc-fpa$(EXESUF): $(ARM_FILES)
	$(CC) -o $@ $< -DTCC_TARGET_ARM $(CFLAGS) $(LIBS)

arm-tcc-fpa-ld$(EXESUF): $(ARM_FILES)
	$(CC) -o $@ $< -DTCC_TARGET_ARM -DLDOUBLE_SIZE=12 $(CFLAGS) $(LIBS)

arm-tcc-vfp$(EXESUF): $(ARM_FILES)
	$(CC) -o $@ $< -DTCC_TARGET_ARM -DTCC_ARM_VFP $(CFLAGS) $(LIBS)

arm-tcc-vfp-eabi$(EXESUF): $(ARM_FILES)
	$(CC) -o $@ $< -DTCC_TARGET_ARM -DTCC_ARM_EABI $(CFLAGS) $(LIBS)

# libtcc generation and test
libtcc.o: $(NATIVE_FILES)
	$(CC) -o $@ -c libtcc.c $(NATIVE_TARGET) $(CFLAGS)

libtcc.a: libtcc.o
	$(AR) rcs $@ $^

libtcc_test$(EXESUF): tests/libtcc_test.c libtcc.a
	$(CC) -o $@ $^ -I. $(CFLAGS) $(LIBS)

libtest: libtcc_test$(EXESUF) $(LIBTCC1)
	./libtcc_test$(EXESUF) lib_path=.

# profiling version
tcc_p$(EXESUF): $(NATIVE_FILES)
	$(CC) -o $@ $< $(NATIVE_TARGET) $(CFLAGS_P) $(LIBS_P)

# windows utilities
tiny_impdef$(EXESUF): win32/tools/tiny_impdef.c
	$(CC) -o $@ $< $(CFLAGS)
tiny_libmaker$(EXESUF): win32/tools/tiny_libmaker.c
	$(CC) -o $@ $< $(CFLAGS)

# TinyCC runtime libraries
LIBTCC1_OBJS=libtcc1.o
LIBTCC1_CC=$(CC)
VPATH+=lib
ifdef CONFIG_WIN32
# for windows, we must use TCC because we generate ELF objects
LIBTCC1_OBJS+=crt1.o wincrt1.o dllcrt1.o dllmain.o chkstk.o
LIBTCC1_CC=./tcc.exe -Bwin32 -DTCC_TARGET_PE
VPATH+=win32/lib
endif
ifeq ($(ARCH),i386)
LIBTCC1_OBJS+=alloca86.o alloca86-bt.o
endif

%.o: %.c
	$(LIBTCC1_CC) -o $@ -c $< -O2 -Wall

%.o: %.S
	$(LIBTCC1_CC) -o $@ -c $<

libtcc1.a: $(LIBTCC1_OBJS)
	$(AR) rcs $@ $^

bcheck.o: bcheck.c
	$(CC) -o $@ -c $< -O2 -Wall

# install
TCC_INCLUDES = stdarg.h stddef.h stdbool.h float.h varargs.h tcclib.h
INSTALL=install

ifndef CONFIG_WIN32
install: $(PROGS) $(LIBTCC1) $(BCHECK_O) libtcc.a tcc.1 tcc-doc.html
	mkdir -p "$(bindir)"
	$(INSTALL) -s -m755 $(PROGS) "$(bindir)"
	mkdir -p "$(mandir)/man1"
	$(INSTALL) tcc.1 "$(mandir)/man1"
	mkdir -p "$(tccdir)"
	mkdir -p "$(tccdir)/include"
ifneq ($(LIBTCC1),)
	$(INSTALL) -m644 $(LIBTCC1) "$(tccdir)"
endif
ifneq ($(BCHECK_O),)
	$(INSTALL) -m644 $(BCHECK_O) "$(tccdir)"
endif
	$(INSTALL) -m644 $(addprefix include/,$(TCC_INCLUDES)) "$(tccdir)/include"
	mkdir -p "$(docdir)"
	$(INSTALL) -m644 tcc-doc.html "$(docdir)"
	mkdir -p "$(libdir)"
	$(INSTALL) -m644 libtcc.a "$(libdir)"
	mkdir -p "$(includedir)"
	$(INSTALL) -m644 libtcc.h "$(includedir)"

uninstall:
	rm -fv $(foreach P,$(PROGS),"$(bindir)/$P")
	rm -fv $(foreach P,$(LIBTCC1) $(BCHECK_O),"$(tccdir)/$P")
	rm -fv $(foreach P,$(TCC_INCLUDES),"$(tccdir)/include/$P")
	rm -fv "$(docdir)/tcc-doc.html" "$(mandir)/man1/tcc.1"
	rm -fv "$(libdir)/libtcc.a" "$(includedir)/libtcc.h"

else
install: $(PROGS) $(LIBTCC1) libtcc.a tcc-doc.html
	mkdir -p "$(tccdir)"
	mkdir -p "$(tccdir)/lib"
	mkdir -p "$(tccdir)/include"
	mkdir -p "$(tccdir)/examples"
	mkdir -p "$(tccdir)/doc"
	mkdir -p "$(tccdir)/libtcc"
	$(INSTALL) -s -m755 $(PROGS) "$(tccdir)"
	$(INSTALL) -m644 $(LIBTCC1) win32/lib/*.def "$(tccdir)/lib"
	cp -r win32/include/. "$(tccdir)/include"
	cp -r win32/examples/. "$(tccdir)/examples"
#	$(INSTALL) -m644 $(addprefix include/,$(TCC_INCLUDES)) "$(tccdir)/include"
	$(INSTALL) -m644 tcc-doc.html win32/tcc-win32.txt "$(tccdir)/doc"
	$(INSTALL) -m644 libtcc.a libtcc.h "$(tccdir)/libtcc"
endif

# documentation and man page
tcc-doc.html: tcc-doc.texi
	-texi2html -monolithic -number $<

tcc.1: tcc-doc.texi
	-./texi2pod.pl $< tcc.pod
	-pod2man --section=1 --center=" " --release=" " tcc.pod > $@

# tar release (use 'make -k tar' on a checkouted tree)
TCC-VERSION=tcc-$(shell cat VERSION)
tar:
	rm -rf /tmp/$(TCC-VERSION)
	cp -r . /tmp/$(TCC-VERSION)
	( cd /tmp ; tar zcvf ~/$(TCC-VERSION).tar.gz $(TCC-VERSION) --exclude CVS )
	rm -rf /tmp/$(TCC-VERSION)

# in tests subdir
test clean :
	$(MAKE) -C tests $@

# clean
clean: local_clean
local_clean:
	rm -vf $(PROGS) tcc_p$(EXESUF) tcc.pod *~ *.o *.a *.out libtcc_test$(EXESUF)

distclean: clean
	rm -vf config.h config.mak config.texi tcc.1 tcc-doc.html

endif # ifeq ($(TOP),.)
