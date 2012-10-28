#
# Tiny C Compiler Makefile
#

TOP ?= .
include $(TOP)/config.mak

CFLAGS_P=$(CFLAGS) -pg -static -DCONFIG_TCC_STATIC
LIBS_P=

ifneq ($(GCC_MAJOR),2)
CFLAGS+=-fno-strict-aliasing
ifneq ($(GCC_MAJOR),3)
CFLAGS+=-Wno-pointer-sign -Wno-sign-compare -D_FORTIFY_SOURCE=0
endif
endif

ifneq ($(TARGETOS),Darwin)
ifeq ($(ARCH),i386)
CFLAGS+=-mpreferred-stack-boundary=2
ifeq ($(GCC_MAJOR),2)
CFLAGS+=-m386 -malign-functions=0
else
CFLAGS+=-march=i386 -falign-functions=0
endif
endif
endif

ifdef CONFIG_WIN64
CONFIG_WIN32=yes
endif

ifndef CONFIG_WIN32
LIBS=-lm
ifndef CONFIG_NOLDL
LIBS+=-ldl
endif
endif

ifeq ($(ARCH),i386)
NATIVE_DEFINES=-DTCC_TARGET_I386 
NATIVE_DEFINES+=$(if $(wildcard /lib/i386-linux-gnu),-DCONFIG_MULTIARCHDIR=\"i386-linux-gnu\")
NATIVE_DEFINES+=$(if $(wildcard /lib/i386-kfreebsd-gnu),-DCONFIG_MULTIARCHDIR=\"i386-kfreebsd-gnu\")
NATIVE_DEFINES+=$(if $(wildcard /lib/i386-gnu),-DCONFIG_MULTIARCHDIR=\"i386-gnu\")
CFLAGS+=-m32
else
ifeq ($(ARCH),x86-64)
NATIVE_DEFINES=-DTCC_TARGET_X86_64
CFLAGS+=-m64
NATIVE_DEFINES+=$(if $(wildcard /usr/lib64),-DCONFIG_LDDIR=\"lib64\")
NATIVE_DEFINES+=$(if $(wildcard /lib/x86_64-linux-gnu),-DCONFIG_MULTIARCHDIR=\"x86_64-linux-gnu\")
NATIVE_DEFINES+=$(if $(wildcard /lib/x86_64-kfreebsd-gnu),-DCONFIG_MULTIARCHDIR=\"x86_64-kfreebsd-gnu\")
endif
endif

ifeq ($(ARCH),arm)
NATIVE_DEFINES=-DTCC_TARGET_ARM
NATIVE_DEFINES+=-DWITHOUT_LIBTCC
NATIVE_DEFINES+=$(if $(wildcard /lib/ld-linux.so.3),-DTCC_ARM_EABI)
ifneq (,$(wildcard /lib/arm-linux-gnueabi))
NATIVE_DEFINES+=-DCONFIG_MULTIARCHDIR=\"arm-linux-gnueabi\"
else
NATIVE_DEFINES+=$(if $(wildcard /lib/arm-linux-gnueabihf),-DCONFIG_MULTIARCHDIR=\"arm-linux-gnueabihf\" -DTCC_ARM_HARDFLOAT)
endif
NATIVE_DEFINES+=$(if $(shell grep -l "^Features.* \(vfp\|iwmmxt\) " /proc/cpuinfo),-DTCC_ARM_VFP)
endif

ifdef CONFIG_WIN32
NATIVE_DEFINES+=-DTCC_TARGET_PE
endif

ifneq ($(wildcard /lib/ld-uClibc.so.0),)
NATIVE_DEFINES+=-DTCC_UCLIBC
CONFIG_UCLIBC = 1
endif

# make libtcc as static or dynamic library?
ifdef DISABLE_STATIC
LIBTCC=libtcc.so.1.0
LINK_LIBTCC=-Wl,-rpath,"$(libdir)"
ifdef DISABLE_RPATH
LINK_LIBTCC=
endif
else
LIBTCC=libtcc.a
LINK_LIBTCC=
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

ifdef CONFIG_WIN64
PROGS+=tiny_impdef$(EXESUF) tiny_libmaker$(EXESUF)
NATIVE_FILES=$(WIN64_FILES)
PROGS_CROSS=$(WIN32_CROSS) $(I386_CROSS) $(X64_CROSS) $(ARM_CROSS) $(C67_CROSS)
LIBTCC1_CROSS=lib/i386-win32/libtcc1.a
LIBTCC1=libtcc1.a
else
ifdef CONFIG_WIN32
PROGS+=tiny_impdef$(EXESUF) tiny_libmaker$(EXESUF)
NATIVE_FILES=$(WIN32_FILES)
PROGS_CROSS=$(WIN64_CROSS) $(I386_CROSS) $(X64_CROSS) $(ARM_CROSS) $(C67_CROSS)
LIBTCC1_CROSS=lib/x86_64-win32/libtcc1.a
LIBTCC1=libtcc1.a
else
ifeq ($(ARCH),i386)
NATIVE_FILES=$(I386_FILES)
PROGS_CROSS=$(X64_CROSS) $(WIN32_CROSS) $(WIN64_CROSS) $(ARM_CROSS) $(C67_CROSS)
LIBTCC1_CROSS=lib/i386-win32/libtcc1.a lib/x86_64-win32/libtcc1.a
LIBTCC1=libtcc1.a
BCHECK_O=bcheck.o
else
ifeq ($(ARCH),x86-64)
NATIVE_FILES=$(X86_64_FILES)
PROGS_CROSS=$(I386_CROSS) $(WIN32_CROSS) $(WIN64_CROSS) $(ARM_CROSS) $(C67_CROSS)
LIBTCC1_CROSS=lib/i386-win32/libtcc1.a lib/x86_64-win32/libtcc1.a lib/i386/libtcc1.a
LIBTCC1=libtcc1.a
else
ifeq ($(ARCH),arm)
NATIVE_FILES=$(ARM_FILES)
PROGS_CROSS=$(I386_CROSS) $(X64_CROSS) $(WIN32_CROSS) $(WIN64_CROSS) $(C67_CROSS)
endif
endif
endif
endif
endif

ifdef CONFIG_UCLIBC
BCHECK_O=
endif
ifeq ($(TARGETOS),Darwin)
BCHECK_O=
PROGS+=tiny_libmaker$(EXESUF)
endif

ifdef CONFIG_USE_LIBGCC
LIBTCC1=
endif

TCCLIBS = $(LIBTCC1) $(LIBTCC) $(BCHECK_O)
TCCDOCS = tcc.1 tcc-doc.html tcc-doc.info

ifdef CONFIG_CROSS
PROGS+=$(PROGS_CROSS)
TCCLIBS+=$(LIBTCC1_CROSS)
endif

all: $(PROGS) $(TCCLIBS) $(TCCDOCS)

# Host Tiny C Compiler
tcc$(EXESUF): tcc.o $(LIBTCC)
	$(CC) -o $@ $^ $(LIBS) $(CFLAGS) $(LDFLAGS) $(LINK_LIBTCC)

# Cross Tiny C Compilers
%-tcc$(EXESUF):
	$(CC) -o $@ tcc.c -DONE_SOURCE $(DEFINES) $(CFLAGS) $(LIBS) $(LDFLAGS)

$(I386_CROSS): DEFINES = -DTCC_TARGET_I386 \
    -DCONFIG_TCCDIR="\"$(tccdir)/i386\""
$(X64_CROSS): DEFINES = -DTCC_TARGET_X86_64
$(WIN32_CROSS): DEFINES = -DTCC_TARGET_I386 -DTCC_TARGET_PE \
    -DCONFIG_TCCDIR="\"$(tccdir)/win32\"" \
    -DCONFIG_TCC_LIBPATHS="\"{B}/lib/32;{B}/lib\""
$(WIN64_CROSS): DEFINES = -DTCC_TARGET_X86_64 -DTCC_TARGET_PE \
    -DCONFIG_TCCDIR="\"$(tccdir)/win32\"" \
    -DCONFIG_TCC_LIBPATHS="\"{B}/lib/64;{B}/lib\""
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
ifndef ONE_SOURCE
LIBTCC_OBJ = $(filter-out tcc.o,$(patsubst %.c,%.o,$(filter %.c,$(NATIVE_FILES))))
LIBTCC_INC = $(filter %.h,$(CORE_FILES)) $(filter-out $(CORE_FILES),$(NATIVE_FILES))
else
LIBTCC_OBJ = libtcc.o
LIBTCC_INC = $(NATIVE_FILES)
libtcc.o : NATIVE_DEFINES += -DONE_SOURCE
endif

$(LIBTCC_OBJ) tcc.o : %.o : %.c $(LIBTCC_INC)
	$(CC) -o $@ -c $< $(NATIVE_DEFINES) $(CFLAGS)

libtcc.a: $(LIBTCC_OBJ)
	$(AR) rcs $@ $^

libtcc.so.1.0: $(LIBTCC_OBJ)
	$(CC) -shared -Wl,-soname,$@ -o $@ $^

libtcc.so.1.0: CFLAGS+=-fPIC

# profiling version
tcc_p$(EXESUF): $(NATIVE_FILES)
	$(CC) -o $@ $< $(NATIVE_DEFINES) $(CFLAGS_P) $(LIBS_P)

# windows utilities
tiny_impdef$(EXESUF): win32/tools/tiny_impdef.c
	$(CC) -o $@ $< $(CFLAGS)
tiny_libmaker$(EXESUF): win32/tools/tiny_libmaker.c
	$(CC) -o $@ $< $(CFLAGS)

# TinyCC runtime libraries
libtcc1.a : FORCE
	@$(MAKE) -C lib native
lib/%/libtcc1.a : FORCE $(PROGS_CROSS)
	@$(MAKE) -C lib cross TARGET=$*
bcheck.o : lib/bcheck.c
	gcc -c $< -o $@ $(CFLAGS)
FORCE:

# install
TCC_INCLUDES = stdarg.h stddef.h stdbool.h float.h varargs.h tcclib.h
INSTALL=install
ifdef STRIP_BINARIES
INSTALLBIN=$(INSTALL) -s
else
INSTALLBIN=$(INSTALL)
endif

ifndef CONFIG_WIN32
install: $(PROGS) $(TCCLIBS) $(TCCDOCS)
	mkdir -p "$(bindir)"
ifeq ($(CC),tcc)
	$(INSTALL) -m755 $(PROGS) "$(bindir)"
else
	$(INSTALLBIN) -m755 $(PROGS) "$(bindir)"
endif
	mkdir -p "$(mandir)/man1"
	-$(INSTALL) tcc.1 "$(mandir)/man1"
	mkdir -p "$(infodir)"
	-$(INSTALL) tcc-doc.info "$(infodir)"
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
	$(INSTALL) -m755 $(LIBTCC) "$(libdir)"
ifdef DISABLE_STATIC
	ln -sf "$(ln_libdir)/libtcc.so.1.0" "$(libdir)/libtcc.so.1"
	ln -sf "$(ln_libdir)/libtcc.so.1.0" "$(libdir)/libtcc.so"
endif
	mkdir -p "$(includedir)"
	$(INSTALL) -m644 libtcc.h "$(includedir)"
	mkdir -p "$(docdir)"
	-$(INSTALL) -m644 tcc-doc.html "$(docdir)"
ifdef CONFIG_CROSS
	mkdir -p "$(tccdir)/win32/lib/32"
	mkdir -p "$(tccdir)/win32/lib/64"
ifeq ($(ARCH),x86-64)
	mkdir -p "$(tccdir)/i386"
	$(INSTALL) -m644 lib/i386/libtcc1.a "$(tccdir)/i386"
	cp -r "$(tccdir)/include" "$(tccdir)/i386"
endif
	$(INSTALL) -m644 win32/lib/*.def "$(tccdir)/win32/lib"
	$(INSTALL) -m644 lib/i386-win32/libtcc1.a "$(tccdir)/win32/lib/32"
	$(INSTALL) -m644 lib/x86_64-win32/libtcc1.a "$(tccdir)/win32/lib/64"
	cp -r win32/include/. "$(tccdir)/win32/include"
	cp -r include/. "$(tccdir)/win32/include"
endif

uninstall:
	rm -fv $(foreach P,$(PROGS),"$(bindir)/$P")
	rm -fv $(foreach P,$(LIBTCC1) $(BCHECK_O),"$(tccdir)/$P")
	rm -fv $(foreach P,$(TCC_INCLUDES),"$(tccdir)/include/$P")
	rm -fv "$(docdir)/tcc-doc.html" "$(mandir)/man1/tcc.1" "$(infodir)/tcc-doc.info"
	rm -fv "$(libdir)/$(LIBTCC)" "$(includedir)/libtcc.h"
	rm -fv "$(libdir)/libtcc.so*"
	rm -rf "$(tccdir)/win32"
	-rmdir $(tccdir)/include
ifeq ($(ARCH),x86-64)
	rm -rf "$(tccdir)/i386"
endif
else
# on windows
install: $(PROGS) $(TCCLIBS) $(TCCDOCS)
	mkdir -p "$(tccdir)"
	mkdir -p "$(tccdir)/lib"
	mkdir -p "$(tccdir)/include"
	mkdir -p "$(tccdir)/examples"
	mkdir -p "$(tccdir)/doc"
	mkdir -p "$(tccdir)/libtcc"
	$(INSTALLBIN) -m755 $(PROGS) "$(tccdir)"
	$(INSTALL) -m644 $(LIBTCC1) win32/lib/*.def "$(tccdir)/lib"
	cp -r win32/include/. "$(tccdir)/include"
	cp -r win32/examples/. "$(tccdir)/examples"
	$(INSTALL) -m644 $(addprefix include/,$(TCC_INCLUDES)) "$(tccdir)/include"
	$(INSTALL) -m644 tcc-doc.html win32/tcc-win32.txt "$(tccdir)/doc"
	$(INSTALL) -m644 $(LIBTCC) libtcc.h "$(tccdir)/libtcc"
ifdef CONFIG_CROSS
	mkdir -p "$(tccdir)/lib/32"
	mkdir -p "$(tccdir)/lib/64"
	-$(INSTALL) -m644 lib/i386-win32/libtcc1.a "$(tccdir)/lib/32"
	-$(INSTALL) -m644 lib/x86_64-win32/libtcc1.a "$(tccdir)/lib/64"
endif

uninstall:
	rm -rfv "$(tccdir)/*"
endif

# documentation and man page
tcc-doc.html: tcc-doc.texi
	-texi2html -monolithic -number $<

tcc.1: tcc-doc.texi
	-./texi2pod.pl $< tcc.pod
	-pod2man --section=1 --center=" " --release=" " tcc.pod > $@

tcc-doc.info: tcc-doc.texi
	-makeinfo tcc-doc.texi

.PHONY: all clean tar distclean install uninstall FORCE

# tar release (use 'make -k tar' on a checkouted tree)
TCC-VERSION=tcc-$(shell cat VERSION)
tar:
	rm -rf /tmp/$(TCC-VERSION)
	cp -r . /tmp/$(TCC-VERSION)
	( cd /tmp ; tar zcvf ~/$(TCC-VERSION).tar.gz $(TCC-VERSION) --exclude CVS )
	rm -rf /tmp/$(TCC-VERSION)

# in tests subdir
export LIBTCC1

%est:
	$(MAKE) -C tests2 $@
	$(MAKE) -C tests $@

clean:
	rm -vf $(PROGS) tcc_p$(EXESUF) tcc.pod *~ *.o *.a *.so* *.out *.exe libtcc_test$(EXESUF)
	$(MAKE) -C tests2 $@
	$(MAKE) -C tests $@
ifneq ($(LIBTCC1),)
	$(MAKE) -C lib $@
endif

distclean: clean
	rm -vf config.h config.mak config.texi tcc.1 tcc-doc.info tcc-doc.html

config.mak:
	@echo "Please run ./configure."
	@exit 1

endif # ifeq ($(TOP),.)
