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
ifneq ($(GCC_MAJOR),3)
CFLAGS+=-Wno-pointer-sign -Wno-sign-compare -D_FORTIFY_SOURCE=0
endif
endif

ifeq ($(ARCH),i386)
CFLAGS+=-mpreferred-stack-boundary=2
ifeq ($(GCC_MAJOR),2)
CFLAGS+=-m386 -malign-functions=0
else
CFLAGS+=-march=i386 -falign-functions=0
endif
endif

ifndef CONFIG_WIN32
LIBS=-lm
ifndef CONFIG_NOLDL
LIBS+=-ldl
endif
endif

ifeq ($(ARCH),i386)
NATIVE_DEFINES=-DTCC_TARGET_I386
LIBTCC1=libtcc1.a
BCHECK_O=bcheck.o
ALLOCA_O=alloca86.o alloca86-bt.o
else
ifeq ($(ARCH),x86-64)
NATIVE_DEFINES=-DTCC_TARGET_X86_64
NATIVE_DEFINES+=$(if $(wildcard /lib64/ld-linux-x86-64.so.2),-DTCC_TARGET_X86_64_CENTOS)
LIBTCC1=libtcc1.a
BCHECK_O=
ALLOCA_O=alloca86_64.o
endif
endif

ifeq ($(ARCH),arm)
NATIVE_DEFINES=-DTCC_TARGET_ARM
NATIVE_TARGET=-DWITHOUT_LIBTCC
NATIVE_DEFINES+=$(if $(wildcard /lib/ld-linux.so.3),-DTCC_ARM_EABI)
NATIVE_DEFINES+=$(if $(shell grep -l "^Features.* \(vfp\|iwmmxt\) " /proc/cpuinfo),-DTCC_ARM_VFP)
endif

ifdef CONFIG_WIN32
NATIVE_DEFINES+=-DTCC_TARGET_PE
BCHECK_O=
endif

ifneq ($(wildcard /lib/ld-uClibc.so.0),)
NATIVE_DEFINES+=-DTCC_UCLIBC
BCHECK_O=
endif

ifdef CONFIG_USE_LIBGCC
LIBTCC1=
endif

ifeq ($(TOP),.)

PROGS=tcc$(EXESUF)
I386_CROSS = i386-tcc$(EXESUF)
WIN32_CROSS = i386-win32-tcc$(EXESUF)
WIN64_CROSS = x86_64-win32-tcc$(EXESUF)
WINCE_CROSS = arm-win32-tcc$(EXESUF)
X64_CROSS = x86_64-tcc$(EXESUF)
ARM_FPA_CROSS = arm-fpa-tcc$(EXESUF)
ARM_FPA_LD_CROSS = arm-fpa-ld-tcc$(EXESUF)
ARM_VFP_CROSS = arm-vfp-tcc$(EXESUF)
ARM_EABI_CROSS = arm-eabi-tcc$(EXESUF)
ARM_CROSS = $(ARM_FPA_CROSS) $(ARM_FPA_LD_CROSS) $(ARM_VFP_CROSS) $(ARM_EABI_CROSS)
C67_CROSS = c67-tcc$(EXESUF)

CORE_FILES = tcc.c libtcc.c tccpp.c tccgen.c tccelf.c tccasm.c tccrun.c
CORE_FILES += tcc.h config.h libtcc.h tcctok.h
I386_FILES = $(CORE_FILES) i386-gen.c i386-asm.c i386-asm.h i386-tok.h
WIN32_FILES = $(CORE_FILES) i386-gen.c i386-asm.c i386-asm.h i386-tok.h tccpe.c
WIN64_FILES = $(CORE_FILES) x86_64-gen.c i386-asm.c x86_64-asm.h tccpe.c
WINCE_FILES = $(CORE_FILES) arm-gen.c tccpe.c
X86_64_FILES = $(CORE_FILES) x86_64-gen.c i386-asm.c x86_64-asm.h
ARM_FILES = $(CORE_FILES) arm-gen.c
C67_FILES = $(CORE_FILES) c67-gen.c tcccoff.c

ifdef CONFIG_WIN32
PROGS+=tiny_impdef$(EXESUF) tiny_libmaker$(EXESUF)
NATIVE_FILES=$(WIN32_FILES)
PROGS_CROSS=$(WIN64_CROSS) $(I386_CROSS) $(X64_CROSS) $(ARM_CROSS) $(C67_CROSS)
else
ifeq ($(ARCH),i386)
NATIVE_FILES=$(I386_FILES)
PROGS_CROSS=$(X64_CROSS) $(WIN32_CROSS) $(WIN64_CROSS) $(ARM_CROSS) $(C67_CROSS)
else
ifeq ($(ARCH),x86-64)
NATIVE_FILES=$(X86_64_FILES)
PROGS_CROSS=$(I386_CROSS) $(WIN32_CROSS) $(WIN64_CROSS) $(ARM_CROSS) $(C67_CROSS)
else
ifeq ($(ARCH),arm)
NATIVE_FILES=$(ARM_FILES)
PROGS_CROSS=$(I386_CROSS) $(X64_CROSS) $(WIN32_CROSS) $(WIN64_CROSS) $(C67_CROSS)
endif
endif
endif
endif

# LIBTCCB decides whether libtcc is installed static or dynamic
LIBTCCB=libtcc.a
ifdef DISABLE_STATIC
CFLAGS+=-fPIC
LIBTCCL=-L. -ltcc
LIBTCCB=libtcc.so.1.0
endif
LIBTCCLIBS=$(LIBTCCB)

ifdef CONFIG_CROSS
PROGS+=$(PROGS_CROSS)
# Try to build win32 cross-compiler lib on *nix
ifndef CONFIG_WIN32
LIBTCCLIBS+=win32libcc1
endif
endif

all: $(PROGS) $(LIBTCC1) $(BCHECK_O) $(LIBTCCLIBS) tcc-doc.html tcc.1 tcc-doc.info libtcc_test$(EXESUF)

# Host Tiny C Compiler
tcc$(EXESUF): tcc.o $(LIBTCCB)
	$(CC) -o $@ $^ $(LIBS) $(LIBTCCL)

# Cross Tiny C Compilers
%-tcc$(EXESUF):
	$(CC) -o $@ tcc.c $(DEFINES) $(CFLAGS) $(LIBS)

$(I386_CROSS): DEFINES = -DTCC_TARGET_I386
$(X64_CROSS): DEFINES = -DTCC_TARGET_X86_64
$(WIN32_CROSS): DEFINES = -DTCC_TARGET_I386 -DTCC_TARGET_PE
$(WIN64_CROSS): DEFINES = -DTCC_TARGET_X86_64 -DTCC_TARGET_PE
$(WINCE_CROSS): DEFINES = -DTCC_TARGET_PE
$(C67_CROSS): DEFINES = -DTCC_TARGET_C67
$(ARM_FPA_CROSS): DEFINES = -DTCC_TARGET_ARM
$(ARM_FPA_LD_CROSS)$(EXESUF): DEFINES = -DTCC_TARGET_ARM -DLDOUBLE_SIZE=12
$(ARM_VFP_CROSS): DEFINES = -DTCC_TARGET_ARM -DTCC_ARM_VFP
$(ARM_EABI_CROSS): DEFINES = -DTCC_TARGET_ARM -DTCC_ARM_EABI

$(I386_CROSS): $(I386_FILES)
$(X64_CROSS): $(X86_64_FILES)
$(WIN32_CROSS): $(WIN32_FILES)
$(WIN64_CROSS): $(WIN64_FILES)
$(WINCE_CROSS): $(WINCE_FILES)
$(C67_CROSS): $(C67_FILES)
$(ARM_FPA_CROSS) $(ARM_FPA_LD_CROSS) $(ARM_VFP_CROSS) $(ARM_EABI_CROSS): $(ARM_FILES)

# libtcc generation and test
ifdef NOTALLINONE
LIBTCC_OBJ = $(filter-out tcc.o,$(patsubst %.c,%.o,$(filter %.c,$(NATIVE_FILES))))
LIBTCC_INC = $(filter %.h,$(CORE_FILES)) $(filter-out $(CORE_FILES),$(NATIVE_FILES))
$(LIBTCC_OBJ) tcc.o : NATIVE_DEFINES += -DNOTALLINONE
else
LIBTCC_OBJ = libtcc.o
LIBTCC_INC = $(NATIVE_FILES)
tcc.o : NATIVE_DEFINES += -DNOTALLINONE
endif

$(LIBTCC_OBJ) tcc.o : %.o : %.c $(LIBTCC_INC)
	$(CC) -o $@ -c $< $(NATIVE_DEFINES) $(CFLAGS)

libtcc.a: $(LIBTCC_OBJ)
	$(AR) rcs $@ $^

libtcc.so.1.0: $(LIBTCC_OBJ)
	$(CC) -shared -Wl,-soname,$@ -o $@ $^
	ln -sf libtcc.so.1.0 libtcc.so.1
	ln -sf libtcc.so.1.0 libtcc.so

libtcc_test$(EXESUF): tests/libtcc_test.c $(LIBTCCB)
	$(CC) -o $@ $^ -I. $(CFLAGS) $(LIBS) $(LIBTCCL)
        
# To build cross-compilers on Linux we must make a fake 32 bit tcc.exe
# and use it to build ELF objects into libtcc1.a which is then
# renamed to tcc1.def in order to have another target in the Makefile
win32libcc1:
	mv config.mak config.mak.bak
	mv config.h config.h.bak
	cp config.h.bak config.h
	cp config.mak.bak config.mak
	echo "ARCH=i386" >> config.mak
	echo "#undef HOST_X86_64" >> config.h
	echo "#define HOST_I386 1" >> config.h
	echo "CFLAGS=-O2 -g -pipe -Wall -m32" >> config.mak
	echo "ARCH=i386" >> config.mak
	$(MAKE) i386-win32-tcc CC=gcc
	cp i386-win32-tcc tcc.exe
	-mv libtcc1.a libtcc1.bak
	$(MAKE) CONFIG_WIN32=1 libtcc1.a
	mv libtcc1.a lib
	-mv libtcc1.bak libtcc1.a
	mv config.mak.bak config.mak
	mv config.h.bak config.h

libtest: libtcc_test$(EXESUF) $(LIBTCC1)
	./libtcc_test$(EXESUF) lib_path=.

# profiling version
tcc_p$(EXESUF): $(NATIVE_FILES)
	$(CC) -o $@ $< $(NATIVE_DEFINES) $(CFLAGS_P) $(LIBS_P)

# windows utilities
tiny_impdef$(EXESUF): win32/tools/tiny_impdef.c
	$(CC) -o $@ $< $(CFLAGS)
tiny_libmaker$(EXESUF): win32/tools/tiny_libmaker.c
	$(CC) -o $@ $< $(CFLAGS)

# TinyCC runtime libraries
LIBTCC1_OBJS=libtcc1.o $(ALLOCA_O)
LIBTCC1_CC=$(CC)
VPATH+=lib

ifdef CONFIG_WIN32
# for windows, we must use TCC because we generate ELF objects
LIBTCC1_OBJS+=crt1.o wincrt1.o dllcrt1.o dllmain.o chkstk.o bcheck.o
LIBTCC1_CC=./tcc.exe -Bwin32 -Iinclude $(NATIVE_DEFINES)
VPATH+=win32/lib
endif

%.o: %.c
	$(LIBTCC1_CC) -o $@ -c $< -O2 -Wall
%.o: %.S
	$(LIBTCC1_CC) -o $@ -c $<

libtcc1.a: $(LIBTCC1_OBJS)
	$(AR) rcs $@ $^

# install
TCC_INCLUDES = stdarg.h stddef.h stdbool.h float.h varargs.h tcclib.h
INSTALL=install

ifndef CONFIG_WIN32
install: $(PROGS) $(LIBTCC1) $(BCHECK_O) $(LIBTCCLIBS) tcc.1 tcc-doc.info tcc-doc.html
	mkdir -p "$(bindir)"
	$(INSTALL) -s -m755 $(PROGS) "$(bindir)"
	mkdir -p "$(mandir)/man1"
	$(INSTALL) tcc.1 "$(mandir)/man1"
	mkdir -p $(infodir)
	$(INSTALL)  tcc-doc.info "$(infodir)"
	mkdir -p "$(tccdir)"
	mkdir -p "$(tccdir)/include"
ifneq ($(LIBTCC1),)
	$(INSTALL) -m644 $(LIBTCC1) "$(tccdir)"
endif
ifneq ($(BCHECK_O),)
	$(INSTALL) -m644 $(BCHECK_O) "$(tccdir)"
endif
	$(INSTALL) -m644 $(addprefix include/,$(TCC_INCLUDES)) "$(tccdir)/include"
	mkdir -p "$(libdir)"
	$(INSTALL) -m755 $(LIBTCCB) "$(libdir)"
ifdef DISABLE_STATIC
	ln -sf "$(ln_libdir)/libtcc.so.1.0" "$(libdir)/libtcc.so.1"
	ln -sf "$(ln_libdir)/libtcc.so.1.0" "$(libdir)/libtcc.so"
endif
	mkdir -p "$(includedir)"
	$(INSTALL) -m644 libtcc.h "$(includedir)"
	mkdir -p "$(docdir)"
	$(INSTALL) -m644 tcc-doc.html "$(docdir)"
ifdef CONFIG_CROSS
	mkdir -p "$(tccdir)/lib"
	$(INSTALL) -m644 win32/lib/*.def lib/libtcc1.a "$(tccdir)/lib"
	cp -r win32/include/. "$(tccdir)/include"
	cp -r win32/examples/. "$(tccdir)/examples"
endif

uninstall:
	rm -fv $(foreach P,$(PROGS),"$(bindir)/$P")
	rm -fv $(foreach P,$(LIBTCC1) $(BCHECK_O),"$(tccdir)/$P")
	rm -fv $(foreach P,$(TCC_INCLUDES),"$(tccdir)/include/$P")
	rm -fv "$(docdir)/tcc-doc.html" "$(mandir)/man1/tcc.1"
	rm -fv "$(libdir)/$(LIBTCCB)" "$(includedir)/libtcc.h"
ifdef DISABLE_STATIC
	rm -fv "$(libdir)/libtcc.so*"
endif
ifdef CONFIG_CROSS
	rm -rf "$(tccdir)/include"
	rm -rf "$(tccdir)/lib"
	rm -rf "$(tccdir)/examples"
endif
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
	$(INSTALL) -m644 $(addprefix include/,$(TCC_INCLUDES)) "$(tccdir)/include"
	$(INSTALL) -m644 tcc-doc.html win32/tcc-win32.txt "$(tccdir)/doc"
	$(INSTALL) -m644 $(LIBTCCB) libtcc.h "$(tccdir)/libtcc"
endif

# documentation and man page
tcc-doc.html: tcc-doc.texi
	-texi2html -monolithic -number $<

tcc.1: tcc-doc.texi
	-./texi2pod.pl $< tcc.pod
	-pod2man --section=1 --center=" " --release=" " tcc.pod > $@

tcc-doc.info: tcc-doc.texi
	makeinfo tcc-doc.texi

# tar release (use 'make -k tar' on a checkouted tree)
TCC-VERSION=tcc-$(shell cat VERSION)
tar:
	rm -rf /tmp/$(TCC-VERSION)
	cp -r . /tmp/$(TCC-VERSION)
	( cd /tmp ; tar zcvf ~/$(TCC-VERSION).tar.gz $(TCC-VERSION) --exclude CVS )
	rm -rf /tmp/$(TCC-VERSION)

# in tests subdir
%est:
	$(MAKE) -C tests $@

clean:
	rm -vf $(PROGS) tcc_p$(EXESUF) tcc.pod *~ *.o *.a *.out *.so* *.exe libtcc_test$(EXESUF) lib/libtcc1.a
	$(MAKE) -C tests $@

distclean: clean
	rm -vf config.h config.mak config.texi tcc.1 tcc-doc.info tcc-doc.html

config.mak:
	@echo Running configure ...
	@./configure

endif # ifeq ($(TOP),.)
