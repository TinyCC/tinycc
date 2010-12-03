#
# Tiny C Compiler Makefile
#

TOP ?= .
include $(TOP)/config.mak
TCC=./tcc
LIBTCC1_DIR= $(TOP)
CFLAGS+=-g -Wall
CFLAGS_P=$(CFLAGS) -pg -static -DCONFIG_TCC_STATIC
LIBS_P=

LIBTCC1=libtcc1.a
ifdef CONFIG_USE_LIBGCC
    LIBTCC1=
endif

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
    LIBTCC1_DIR=lib
    BCHECK_O=bcheck.o
    ALLOCA_O=alloca86.o alloca86-bt.o
else ifeq ($(ARCH),x86-64)
    NATIVE_DEFINES=-DTCC_TARGET_X86_64
    NATIVE_DEFINES+=$(if $(wildcard /lib64/ld-linux-x86-64.so.2),-DTCC_TARGET_X86_64_CENTOS)
    LIBTCC1_DIR=lib64
    BCHECK_O=
    ALLOCA_O=alloca86_64.o
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
else ifeq ($(ARCH),i386)
    NATIVE_FILES=$(I386_FILES)
    PROGS_CROSS=$(X64_CROSS) $(WIN32_CROSS) $(WIN64_CROSS) $(ARM_CROSS) $(C67_CROSS)
else ifeq ($(ARCH),x86-64)
    NATIVE_FILES=$(X86_64_FILES)
    PROGS_CROSS= $(WIN32_CROSS) $(I386_CROSS) $(WIN64_CROSS) $(ARM_CROSS) $(C67_CROSS)
else ifeq ($(ARCH),arm)
    NATIVE_FILES=$(ARM_FILES)
    PROGS_CROSS=$(I386_CROSS) $(X64_CROSS) $(WIN32_CROSS) $(WIN64_CROSS) $(C67_CROSS)
endif

# LIBTCCB decides whether libtcc is built static or dynamic
LIBTCCB=libtcc.a
ifdef DISABLE_STATIC
    CFLAGS+=-fPIC
    LIBTCCL=-L. -ltcc
    LIBTCCB=libtcc.so.1.0
endif
LIBTCCLIBS=$(LIBTCCB)

# conditionally make win32/lib/xx/libtcc1.a cross compiler lib archives
ifdef CONFIG_CROSS
    PROGS+=$(PROGS_CROSS)
    ifdef CONFIG_WIN32
        LIBTCCLIBS+=win32
    else ifdef CONFIG_WIN64
        LIBTCCLIBS+=win64
    else
        LIBTCCLIBS+=$(ARCH)
    endif
endif

all: $(PROGS) $(LIBTCC1_DIR)/$(LIBTCC1) $(BCHECK_O) $(LIBTCCLIBS) tcc-doc.html tcc.1 tcc-doc.info libtcc_test$(EXESUF)

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
LIBTCC1_OBJS=$(LIBTCC1_DIR)/libtcc1.o $(ALLOCA_O)
LIBTCC1_CC=$(CC)
VPATH+=lib

WINDLLS=$(LIBTCC1_DIR)/crt1.o $(LIBTCC1_DIR)/wincrt1.o $(LIBTCC1_DIR)/dllcrt1.o $(LIBTCC1_DIR)/dllmain.o
ifdef CONFIG_WIN32
    # for windows, we must use TCC because we generate ELF objects
    LIBTCC1_OBJS+=$(WINDLLS) $(LIBTCC1_DIR)/chkstk.o $(LIBTCC1_DIR)/bcheck.o
    LIBTCC1_CC=./$(TCC)$(EXESUF) -B. -Iinclude $(NATIVE_DEFINES)
    VPATH+=win32/lib
endif

ifdef CONFIG_WIN64
    # windows 64: fixme: chkstk.o bcheck.o fails, not included
    # windows 64: fixme: alloca86_64 is broken
    LIBTCC1_OBJS+=$(WINDLLS)
    LIBTCC1_CC=./$(TCC)$(EXESUF) -B. -Iinclude $(NATIVE_DEFINES)
    VPATH+=win32/lib
endif
#~ win32/lib/64/libtcc1.o:
	#~ $(CC) -o $@ -c $< $(NATIVE_DEFINES) $(CFLAGS)
$(LIBTCC1_DIR)/%.o: %.c
	$(LIBTCC1_CC) -o $@ -c $< -O2 -Wall
$(LIBTCC1_DIR)/%.o: %.S
	$(LIBTCC1_CC) -o $@ -c $<

$(LIBTCC1_DIR)/libtcc1.a: $(LIBTCC1_OBJS)
	$(AR) rcs $@ $^
	$(RM) $^

#recursively build cross-compiler lib archives for each arch
win32: $(TCC)$(EXESUF) $(WIN64_CROSS)
#testing: bleeding: cross-cross (./configure --enable-mingw32 --enable-cross)
#fixme: The windows exe cross-compilers still use Linux libc
#fixme: thus cygwin or some such needed to even run them on Windows
	$(MAKE) CC=./$(I386_CROSS) ARCH=i386 LIBTCC1_DIR=lib lib/libtcc1.a
	$(MAKE) CC=./$(X64_CROSS) LIBTCC1_DIR=lib64 \
    ARCH=x86-64 LIBTCC1_OBJS="lib/libtcc1.o alloca86_64.o" lib64/libtcc1.a
	$(MAKE) CC=./$(WIN64_CROSS) CONFIG_CROSS= TCC=./x86_64-win32-tcc \
    ARCH=x86-64 LIBTCC1_DIR=win32/lib/64 CONFIG_WIN64=1 win32/lib/64/libtcc1.a
    #todo: add arm, etc cross lib archives

win64: $(PROGS_CROSS)
#testing: bleeding: mingw64 cross-cross lib stuff would go here
	$(MAKE) TCC=./$(WIN32_CROSS) CC=./$(WIN32_CROSS) LIBTCC1_DIR=win32/lib/32 \
    ARCH=i386 CONFIG_WIN32=1 win32/lib/32/libtcc1.a
	$(MAKE) CC=./$(I386_CROSS) LIBTCC1_DIR=lib ARCH=i386 lib/libtcc1.a
	$(MAKE) CC=./$(X64_CROSS) LIBTCC1_DIR=lib64 ARCH=x86-64 lib64/libtcc1.a

i386: $(PROGS_CROSS)
#testing: unstable: build cross-compiler libs on i386 platform
	$(MAKE) TCC=./$(WIN32_CROSS) CC=./$(WIN32_CROSS) LIBTCC1_DIR=win32/lib/32 \
    ARCH=i386 CONFIG_WIN32=1 win32/lib/32/libtcc1.a
	$(MAKE) TCC=./$(WIN64_CROSS) CC=./$(WIN64_CROSS) LIBTCC1_DIR=win32/lib/64 \
    ARCH=x86-64 CONFIG_WIN64=1 win32/lib/64/libtcc1.a
	$(MAKE) CC=./$(X64_CROSS) LIBTCC1_DIR=lib64 ARCH=x86-64 lib64/libtcc1.a

x86-64: $(PROGS_CROSS)
#stable: build cross-compiler lib archives on x86_64 platform
	$(MAKE) TCC=./$(WIN32_CROSS) CC=./$(WIN32_CROSS) LIBTCC1_DIR=win32/lib/32 \
    ARCH=i386 CONFIG_WIN32=1 win32/lib/32/libtcc1.a
	$(MAKE) TCC=./$(WIN64_CROSS) CC=./$(WIN64_CROSS) LIBTCC1_DIR=win32/lib/64 \
    ARCH=x86-64 CONFIG_WIN64=1 win32/lib/64/libtcc1.a
	$(MAKE) CC=./$(I386_CROSS) LIBTCC1_DIR=lib ARCH=i386 lib/libtcc1.a

# install
TCC_INCLUDES = stdarg.h stddef.h stdbool.h float.h varargs.h tcclib.h
INSTALL=install

ifndef CONFIG_WIN32
install: $(PROGS) $(LIBTCC1) $(BCHECK_O) $(LIBTCCLIBS) tcc.1 tcc-doc.info tcc-doc.html
	mkdir -p "$(bindir)"
	-$(STRIP) $(PROGS)
	$(INSTALL) -m755 $(PROGS) "$(bindir)"
	mkdir -p "$(mandir)/man1"
	$(INSTALL) tcc.1 "$(mandir)/man1"
	mkdir -p $(infodir)
	$(INSTALL)  tcc-doc.info "$(infodir)"
	mkdir -p "$(tccdir)"
	mkdir -p "$(tccdir)/include"
	mkdir -p "$(tccdir)/lib"
	-$(INSTALL) lib/libtcc1.a "$(tccdir)/lib"
	mkdir -p "$(tccdir)/lib64"
	-$(INSTALL) lib64/libtcc1.a "$(tccdir)/lib64"
ifneq ($(BCHECK_O),)
	$(INSTALL) -m644 $(BCHECK_O) "$(tccdir)"
endif
	$(INSTALL) -m644 $(addprefix include/,$(TCC_INCLUDES)) "$(tccdir)/include"
	mkdir -p "$(libdir)"
	-$(INSTALL) -m755 $(LIBTCCB) "$(libdir)"
ifdef DISABLE_STATIC
	ln -sf "$(ln_libdir)/libtcc.so.1.0" "$(libdir)/libtcc.so.1"
	ln -sf "$(ln_libdir)/libtcc.so.1.0" "$(libdir)/libtcc.so"
endif
	mkdir -p "$(includedir)"
	$(INSTALL) -m644 libtcc.h "$(includedir)"
	mkdir -p "$(docdir)"
	$(INSTALL) -m644 tcc-doc.html "$(docdir)"
ifdef CONFIG_CROSS
	mkdir -p "$(tccdir)/win32/lib"
	$(INSTALL) -m644 win32/lib/*.def "$(tccdir)/win32/lib"
	mkdir -p "$(tccdir)/win32/lib/32"
	$(INSTALL) win32/lib/32/libtcc1.a "$(tccdir)/win32/lib/32"
	mkdir -p "$(tccdir)/win32/lib/64"
	$(INSTALL) win32/lib/64/libtcc1.a "$(tccdir)/win32/lib/64"
	cp -r win32/include/. "$(tccdir)/include"
	cp -r win32/examples/. "$(tccdir)/examples"
endif

uninstall:
	rm -fv $(foreach P,$(PROGS),"$(bindir)/$P")
	rm -fv $(foreach P,$(BCHECK_O),"$(tccdir)/$P")
	rm -fv $(foreach P,$(TCC_INCLUDES),"$(tccdir)/include/$P")
	rm -fv "$(docdir)/tcc-doc.html" "$(mandir)/man1/tcc.1"
	rm -fv "$(libdir)/$(LIBTCCB)" "$(includedir)/libtcc.h"
	rm -fv "$(tccdir)/lib/libtcc1.a"
	rm -fv "$(tccdir)/lib64/libtcc1.a"
ifdef DISABLE_STATIC
	rm -fv "$(libdir)/libtcc.so"
	rm -fv "$(libdir)/libtcc.so.1"
	rm -fv "$(libdir)/libtcc.so.1.0"
else
	rm -fv "$(libdir)/libtcc.a"
endif
	-rmdir "$(tccdir)/lib"
	-rmdir "$(tccdir)/lib64"
	-rmdir "$(tccdir)"
ifdef CONFIG_CROSS
	rm -rfv "$(tccdir)/include"
	rm -rfv "$(tccdir)/examples"
	rm -rfv "$(tccdir)/win32"
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
	$(INSTALL) -m644 win32/lib/*.def "$(tccdir)/lib"
	cp -r win32/include/. "$(tccdir)/include"
	cp -r win32/examples/. "$(tccdir)/examples"
	$(INSTALL) -m644 $(addprefix include/,$(TCC_INCLUDES)) "$(tccdir)/include"
	$(INSTALL) -m644 tcc-doc.html win32/tcc-win32.txt "$(tccdir)/doc"
	$(INSTALL) -m644 $(LIBTCCB) libtcc.h "$(tccdir)/libtcc"
	mkdir -p "$(tccdir)/win32/lib/32"
	-$(INSTALL) win32/lib/32/libtcc1.a "$(tccdir)/win32/lib/32"
	mkdir -p "$(tccdir)/win32/lib/64"
	-$(INSTALL) win32/lib/64/libtcc1.a "$(tccdir)/win32/lib/64"
ifdef CONFIG_CROSS
	$(INSTALL) lib/libtcc1.a "$(tccdir)/lib"
	mkdir -p "$(tccdir)/lib64"
	$(INSTALL) lib64/libtcc1.a "$(tccdir)/lib64
endif

endif

# documentation and man page
tcc-doc.html: tcc-doc.texi
	-texi2html -monolithic -number $<

tcc.1: tcc-doc.texi
	-./texi2pod.pl $< tcc.pod
	-pod2man --section=1 --center=" " --release=" " tcc.pod > $@

tcc-doc.info: tcc-doc.texi
	makeinfo tcc-doc.texi

.PHONY: all libtest clean tar distclean install uninstall win32 win64 x86 x86-64

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
	rm -vf $(PROGS) tcc_p$(EXESUF) tcc.pod *~ *.o *.a *.out *.so* *.exe libtcc_test$(EXESUF)
	rm -vf win32/lib/32/libtcc1.a win32/lib/64/libtcc1.a lib64/libtcc1.a lib/libtcc1.a $(WIN32_CROSS)
	$(MAKE) -C tests $@

distclean: clean
	rm -vf config.h config.mak config.texi tcc.1 tcc-doc.info tcc-doc.html

config.mak:
	@echo Running configure ...
	@./configure

endif # ifeq ($(TOP),.)
