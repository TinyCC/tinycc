#
# Tiny C Compiler Makefile
#

TOP ?= .
include $(TOP)/config.mak
VPATH = $(top_srcdir)

CPPFLAGS += -I$(TOP) # for config.h

ifneq (-$(findstring gcc,$(CC))-,-gcc-)
ifeq (-$(findstring clang,$(CC))-,-clang-)
# make clang accept gnuisms in libtcc1.c
CFLAGS+=-fheinous-gnu-extensions
endif
endif

CPPFLAGS_P=$(CPPFLAGS) -DCONFIG_TCC_STATIC
CFLAGS_P=$(CFLAGS) -pg -static
LIBS_P=
LDFLAGS_P=$(LDFLAGS)

ifdef CONFIG_WIN64
CONFIG_WIN32=yes
endif

ifndef CONFIG_WIN32
LIBS=-lm
ifndef CONFIG_NOLDL
LIBS+=-ldl
endif
endif

# make libtcc as static or dynamic library?
ifdef DISABLE_STATIC
ifndef CONFIG_WIN32
LIBTCC=libtcc.so.1.0
else
LIBTCC=libtcc.dll
LIBTCC_DLL=yes
LIBTCC_EXTRA=libtcc.def libtcc.a
endif
LINK_LIBTCC=-Wl,-rpath,"$(libdir)"
ifdef DISABLE_RPATH
LINK_LIBTCC=
endif
else
LIBTCC=libtcc.a
LINK_LIBTCC=
endif

CONFIG_$(ARCH) = yes
NATIVE_DEFINES_$(CONFIG_i386) += -DTCC_TARGET_I386
NATIVE_DEFINES_$(CONFIG_x86-64) += -DTCC_TARGET_X86_64
NATIVE_DEFINES_$(CONFIG_WIN32) += -DTCC_TARGET_PE
NATIVE_DEFINES_$(CONFIG_uClibc) += -DTCC_UCLIBC
NATIVE_DEFINES_$(CONFIG_arm) += -DTCC_TARGET_ARM
NATIVE_DEFINES_$(CONFIG_arm_eabihf) += -DTCC_ARM_EABI -DTCC_ARM_HARDFLOAT
NATIVE_DEFINES_$(CONFIG_arm_eabi) += -DTCC_ARM_EABI
NATIVE_DEFINES_$(CONFIG_arm_vfp) += -DTCC_ARM_VFP
NATIVE_DEFINES_$(CONFIG_arm64) += -DTCC_TARGET_ARM64
NATIVE_DEFINES += $(NATIVE_DEFINES_yes)

ifeq ($(TOP),.)

PROGS=tcc$(EXESUF)
I386_CROSS = i386-linux-gnu-tcc$(EXESUF)
WIN32_CROSS = i386-win-mingw32-tcc$(EXESUF)
WIN64_CROSS = x86_64-win-mingw32-tcc$(EXESUF)
WINCE_CROSS = arm-win-mingw32ce-tcc$(EXESUF)
X64_CROSS = x86_64-linux-gnu-tcc$(EXESUF)
ARM_FPA_CROSS = arm-linux-fpa-tcc$(EXESUF)
ARM_FPA_LD_CROSS = arm-linux-fpa-ld-tcc$(EXESUF)
ARM_VFP_CROSS = arm-linux-gnu-tcc$(EXESUF)
ARM_EABI_CROSS = arm-linux-gnueabi-tcc$(EXESUF)
ARM_EABIHF_CROSS = arm-linux-gnueabihf-tcc$(EXESUF)
ARM_CROSS = $(ARM_FPA_CROSS) $(ARM_FPA_LD_CROSS) $(ARM_VFP_CROSS) $(ARM_EABI_CROSS)
ARM64_CROSS = arm64-tcc$(EXESUF)
C67_CROSS = c67-tcc$(EXESUF)

# Legacy symlinks for cross compilers
$(I386_CROSS)_LINK = i386-tcc$(EXESUF)
$(WIN32_CROSS)_LINK = i386-win-tcc$(EXESUF)
$(WIN64_CROSS)_LINK = x86_64-win-tcc$(EXESUF)
$(WINCE_CROSS)_LINK = arm-win-tcc$(EXESUF)
$(X64_CROSS)_LINK = x86_64-tcc$(EXESUF)
$(ARM_FPA_CROSS)_LINK = arm-fpa-tcc$(EXESUF)
$(ARM_FPA_LD_CROSS)_LINK = arm-fpa-ld-tcc$(EXESUF)
$(ARM_VFP_CROSS)_LINK = arm-vfp-tcc$(EXESUF)
$(ARM_EABI_CROSS)_LINK = arm-eabi-tcc$(EXESUF)

ifeq ($(TARGETOS),Windows)
ifeq ($(ARCH),i386)
PROGS:=$($(WIN32_CROSS)_LINK)
$($(WIN32_CROSS)_LINK)_TCC = yes
endif
ifeq ($(ARCH),x86-64)
PROGS:=$($(WIN64_CROSS)_LINK)
$($(WIN64_CROSS)_LINK)_TCC = yes
endif
endif

ifeq ($(TARGETOS),Linux)
ifeq ($(ARCH),i386)
PROGS:=$($(I386_CROSS)_LINK)
$($(I386_CROSS)_LINK)_TCC = yes
endif
ifeq ($(ARCH),x86-64)
PROGS:=$($(X64_CROSS)_LINK)
$($(X64_CROSS)_LINK)_TCC = yes
endif
endif

CORE_FILES = tcc.c libtcc.c tccpp.c tccgen.c tccelf.c tccasm.c tccrun.c
CORE_FILES += tcc.h config.h libtcc.h tcctok.h
I386_FILES = $(CORE_FILES) i386-gen.c i386-asm.c i386-asm.h i386-tok.h
WIN32_FILES = $(CORE_FILES) i386-gen.c i386-asm.c i386-asm.h i386-tok.h tccpe.c
WIN64_FILES = $(CORE_FILES) x86_64-gen.c i386-asm.c x86_64-asm.h tccpe.c
WINCE_FILES = $(CORE_FILES) arm-gen.c tccpe.c
X86_64_FILES = $(CORE_FILES) x86_64-gen.c i386-asm.c x86_64-asm.h
ARM_FILES = $(CORE_FILES) arm-gen.c
ARM64_FILES = $(CORE_FILES) arm64-gen.c
C67_FILES = $(CORE_FILES) c67-gen.c tcccoff.c

ifdef CONFIG_WIN64
PROGS+=tiny_impdef$(EXESUF)
NATIVE_FILES=$(WIN64_FILES)
PROGS_CROSS=$(WIN32_CROSS) $(I386_CROSS) $(X64_CROSS) $(ARM_CROSS) $(ARM64_CROSS) $(C67_CROSS) $(WINCE_CROSS)
LIBTCC1_CROSS=lib/i386-win/libtcc1.a
LIBTCC1=libtcc1.a
else ifdef CONFIG_WIN32
PROGS+=tiny_impdef$(EXESUF)
NATIVE_FILES=$(WIN32_FILES)
PROGS_CROSS=$(WIN64_CROSS) $(I386_CROSS) $(X64_CROSS) $(ARM_CROSS) $(ARM64_CROSS) $(C67_CROSS) $(WINCE_CROSS)
LIBTCC1_CROSS=lib/x86_64-win/libtcc1.a
LIBTCC1=libtcc1.a
else ifeq ($(ARCH),i386)
NATIVE_FILES=$(I386_FILES)
PROGS_CROSS=$($(X64_CROSS)_LINK) $($(WIN32_CROSS)_LINK) $($(WIN64_CROSS)_LINK) $(ARM_CROSS) $(ARM64_CROSS) $(C67_CROSS) $(WINCE_CROSS)
LIBTCC1_CROSS=lib/i386-win/libtcc1.a lib/x86_64-win/libtcc1.a lib/i386/libtcc1.a lib/x86_64/libtcc1.a \
    lib/arm64/libtcc1.a
LIBTCC1=libtcc1.a
else ifeq ($(ARCH),x86-64)
ifeq ($(TARGETOS),Darwin)
NATIVE_FILES=$(X86_64_FILES)
PROGS_CROSS=$($(I386_CROSS)_LINK) $($(WIN32_CROSS)_LINK) $($(WIN64_CROSS)_LINK) $(ARM_CROSS) $(C67_CROSS) $(WINCE_CROSS)
LIBTCC1_CROSS=lib/i386-win/libtcc1.a lib/x86_64-win/libtcc1.a lib/i386/libtcc1.a lib/x86_64/libtcc1.a
LIBTCC1=libtcc1.a
else
NATIVE_FILES=$(X86_64_FILES)
PROGS_CROSS=$($(I386_CROSS)_LINK) $($(WIN32_CROSS)_LINK) $($(WIN64_CROSS)_LINK) $(ARM_CROSS) $(ARM64_CROSS) $(C67_CROSS) $(WINCE_CROSS)
LIBTCC1_CROSS=lib/i386-win/libtcc1.a lib/x86_64-win/libtcc1.a lib/i386/libtcc1.a lib/x86_64/libtcc1.a \
    lib/arm64/libtcc1.a
LIBTCC1=libtcc1.a
endif
else ifeq ($(ARCH),arm)
NATIVE_FILES=$(ARM_FILES)
PROGS_CROSS=$(I386_CROSS) $(X64_CROSS) $(WIN32_CROSS) $(WIN64_CROSS) $(ARM64_CROSS) $(C67_CROSS) $(WINCE_CROSS)
LIBTCC1=libtcc1.a
LIBTCC1_CROSS=lib/i386-win/libtcc1.a lib/x86_64-win/libtcc1.a lib/i386/libtcc1.a
else ifeq ($(ARCH),arm64)
NATIVE_FILES=$(ARM64_FILES)
PROGS_CROSS=$(I386_CROSS) $(X64_CROSS) $(WIN32_CROSS) $(WIN64_CROSS) $(ARM_CROSS) $(C67_CROSS) $(WINCE_CROSS)
LIBTCC1=libtcc1.a
LIBTCC1_CROSS=lib/i386-win/libtcc1.a lib/x86_64-win/libtcc1.a lib/i386/libtcc1.a
endif
PROGS_CROSS_LINK=$(foreach PROG_CROSS,$(PROGS_CROSS),$($(PROG_CROSS)_LINK))

PROGS+=tiny_libmaker$(EXESUF)

TCCLIBS = $(LIBTCC1) $(LIBTCC) $(LIBTCC_EXTRA)
TCCDOCS = tcc.1 tcc-doc.html tcc-doc.info

ifdef CONFIG_CROSS
PROGS+=$(PROGS_CROSS)
TCCLIBS+=$(LIBTCC1_CROSS)
endif

all: $(PROGS) $(TCCLIBS) $(TCCDOCS)

# Host Tiny C Compiler
tcc$(EXESUF): tcc.o $(LIBTCC)
	$(CC) -o $@ $^ $(LIBS) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $(LINK_LIBTCC)

# Cross Tiny C Compilers
%-tcc$(EXESUF): tcc.c
	$(CC) -o $@ $< -DONE_SOURCE $(if $($@_TCC),$(NATIVE_DEFINES),$(DEFINES)) $(CPPFLAGS) $(CFLAGS) $(LIBS) $(LDFLAGS)
	$(if $($@_LINK),ln -sf $@ $($@_LINK))
	$(if $($@_TCC),ln -sf $@ tcc$(EXESUF))

# profiling version
tcc_p$(EXESUF): $(NATIVE_FILES)
	$(CC) -o $@ $< -DONE_SOURCE $(NATIVE_DEFINES) $(CPPFLAGS_P) $(CFLAGS_P) $(LIBS_P) $(LDFLAGS_P)

$(I386_CROSS) $($(I386_CROSS)_LINK): DEFINES = -DTCC_TARGET_I386
$(X64_CROSS) $($(X64_CROSS)_LINK): DEFINES = -DTCC_TARGET_X86_64
$(WIN32_CROSS) $($(WIN32_CROSS)_LINK): DEFINES = -DTCC_TARGET_I386 -DTCC_TARGET_PE \
    -DCONFIG_TCCDIR="\"$(tccdir)/win32\"" \
    -DCONFIG_TCC_LIBPATHS="\"{B}/lib/32;{B}/lib\""
$(WIN64_CROSS) $($(WIN64_CROSS)_LINK): DEFINES = -DTCC_TARGET_X86_64 -DTCC_TARGET_PE \
    -DCONFIG_TCCDIR="\"$(tccdir)/win32\"" \
    -DCONFIG_TCC_LIBPATHS="\"{B}/lib/64;{B}/lib\""
$(WINCE_CROSS): DEFINES = -DTCC_TARGET_PE
$(C67_CROSS): DEFINES = -DTCC_TARGET_C67
$(ARM_FPA_CROSS): DEFINES = -DTCC_TARGET_ARM
$(ARM_FPA_LD_CROSS)$(EXESUF): DEFINES = -DTCC_TARGET_ARM -DLDOUBLE_SIZE=12
$(ARM_VFP_CROSS): DEFINES = -DTCC_TARGET_ARM -DTCC_ARM_VFP
$(ARM_EABI_CROSS): DEFINES = -DTCC_TARGET_ARM -DTCC_ARM_EABI -DTCC_ARM_VFP
$(ARM64_CROSS): DEFINES = -DTCC_TARGET_ARM64

$(I386_CROSS) $($(I386_CROSS)_LINK): $(I386_FILES)
$(X64_CROSS)  $($(X64_CROSS)_LINK): $(X86_64_FILES)
$(WIN32_CROSS) $($(WIN32_CROSS)_LINK): $(WIN32_FILES)
$(WIN64_CROSS) $($(WIN64_CROSS)_LINK): $(WIN64_FILES)
$(WINCE_CROSS) $($(WINCE_CROSS)_LINK): $(WINCE_FILES)
$(C67_CROSS)  $($(C67_CROSS)_LINK): $(C67_FILES)
$(ARM_FPA_CROSS) $(ARM_FPA_LD_CROSS) $(ARM_VFP_CROSS) $(ARM_EABI_CROSS): $(ARM_FILES)
$($(ARM_FPA_CROSS)_LINK) $($(ARM_FPA_LD_CROSS)_LINK) $($(ARM_VFP_CROSS)_LINK) $($(ARM_EABI_CROSS)_LINK): $(ARM_FILES)
$(ARM64_CROSS): $(ARM64_FILES)

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
	$(CC) -o $@ -c $< $(NATIVE_DEFINES) $(CPPFLAGS) $(CFLAGS)

ifndef LIBTCC_DLL
libtcc.a: $(LIBTCC_OBJ)
	$(AR) rcs $@ $^
endif

libtcc.so.1.0: $(LIBTCC_OBJ)
	$(CC) -shared -Wl,-soname,$@ -o $@ $^ $(LDFLAGS)

libtcc.so.1.0: CFLAGS+=-fPIC

ifdef LIBTCC_DLL
libtcc.dll libtcc.def libtcc.a: $(LIBTCC_OBJ)
	$(CC) -shared $^ -o $@ $(LDFLAGS) -Wl,--output-def,libtcc.def,--out-implib,libtcc.a
endif

# windows utilities
tiny_impdef$(EXESUF): win32/tools/tiny_impdef.c
	$(CC) -o $@ $< $(CPPFLAGS) $(CFLAGS) $(LDFLAGS)
tiny_libmaker$(EXESUF): win32/tools/tiny_libmaker.c
	$(CC) -o $@ $< $(CPPFLAGS) $(CFLAGS) $(LDFLAGS)

# TinyCC runtime libraries
libtcc1.a : FORCE $(PROGS)
	$(MAKE) -C lib native
	if test ! -d $(ARCH); then mkdir $(ARCH); fi
	if test ! -L $(ARCH)/$@; then ln -sf ../$@ $(ARCH)/$@; fi
lib/%/libtcc1.a : FORCE $(PROGS_CROSS)
	$(MAKE) -C lib cross TARGET=$*

FORCE:

# install
TCC_INCLUDES = stdarg.h stddef.h stdbool.h float.h varargs.h
INSTALL=install
ifdef STRIP_BINARIES
INSTALLBIN=$(INSTALL) -s
else
INSTALLBIN=$(INSTALL)
endif

ifdef CONFIG_WIN32
CONFIG_WIN=yes
endif
ifdef CONFIG_WIN64
CONFIG_WIN=yes
endif

install-strip: install
	strip $(foreach PROG,$(PROGS),"$(bindir)"/$(PROG))

ifndef CONFIG_WIN
install: $(PROGS) $(TCCLIBS) $(TCCDOCS)
	mkdir -p "$(bindir)"
	$(INSTALLBIN) -m755 $(PROGS) "$(bindir)"
	cp -P tcc$(EXESUF) "$(bindir)"
	mkdir -p "$(mandir)/man1"
	-$(INSTALL) -m644 tcc.1 "$(mandir)/man1"
	mkdir -p "$(infodir)"
	-$(INSTALL) -m644 tcc-doc.info "$(infodir)"
	mkdir -p "$(tccdir)"
	mkdir -p "$(tccdir)/include"
ifneq ($(LIBTCC1),)
	mkdir -p "$(tccdir)/$(ARCH)"
	$(INSTALL) -m644 $(LIBTCC1) "$(tccdir)/$(ARCH)"
endif
	$(INSTALL) -m644 $(addprefix $(top_srcdir)/include/,$(TCC_INCLUDES)) $(top_srcdir)/tcclib.h "$(tccdir)/include"
	mkdir -p "$(libdir)"
	$(INSTALL) -m644 $(LIBTCC) "$(libdir)"
ifdef DISABLE_STATIC
	ln -sf "$(ln_libdir)/libtcc.so.1.0" "$(libdir)/libtcc.so.1"
	ln -sf "$(ln_libdir)/libtcc.so.1.0" "$(libdir)/libtcc.so"
endif
	mkdir -p "$(includedir)"
	$(INSTALL) -m644 $(top_srcdir)/libtcc.h "$(includedir)"
	mkdir -p "$(docdir)"
	-$(INSTALL) -m644 tcc-doc.html "$(docdir)"
ifdef CONFIG_CROSS
	mkdir -p "$(tccdir)/win32/lib/32"
	mkdir -p "$(tccdir)/win32/lib/64"
	mkdir -p "$(tccdir)/i386"
	mkdir -p "$(tccdir)/x86-64"
ifneq ($(HOST_OS),Darwin)
	mkdir -p "$(tccdir)/arm64"
	$(INSTALL) -m644 lib/arm64/libtcc1.a "$(tccdir)/arm64"
endif
	$(INSTALL) -m644 lib/i386/libtcc1.a "$(tccdir)/i386"
	$(INSTALL) -m644 lib/x86_64/libtcc1.a "$(tccdir)/x86-64"
	$(INSTALL) -m644 $(top_srcdir)/win32/lib/*.def "$(tccdir)/win32/lib"
	$(INSTALL) -m644 lib/i386-win/libtcc1.a "$(tccdir)/win32/lib/32"
	$(INSTALL) -m644 lib/x86_64-win/libtcc1.a "$(tccdir)/win32/lib/64"
	cp -r $(top_srcdir)/win32/include/. "$(tccdir)/win32/include"
	cp -r "$(tccdir)/include" "$(tccdir)/win32"
endif

uninstall:
	rm -fv $(foreach P,$(PROGS),"$(bindir)/$P")
	rm -fv "$(bindir)/tcc$(EXESUF)"
	rm -fv $(foreach P,$(LIBTCC1),"$(tccdir)/$P")
	rm -fv $(foreach P,$(TCC_INCLUDES),"$(tccdir)/include/$P")
	rm -fv "$(mandir)/man1/tcc.1" "$(infodir)/tcc-doc.info"
	rm -fv "$(libdir)/$(LIBTCC)" "$(includedir)/libtcc.h"
	rm -fv "$(libdir)/libtcc.so*"
	rm -rv "$(tccdir)"
	rm -rv "$(docdir)"
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
	$(INSTALLBIN) -m755 tcc.exe "$(tccdir)"
	$(INSTALL) -m644 $(LIBTCC1) $(top_srcdir)/win32/lib/*.def "$(tccdir)/lib"
	cp -r $(top_srcdir)/win32/include/. "$(tccdir)/include"
	cp -r $(top_srcdir)/win32/examples/. "$(tccdir)/examples"
	$(INSTALL) -m644 $(addprefix $(top_srcdir)/include/,$(TCC_INCLUDES)) $(top_srcdir)/tcclib.h "$(tccdir)/include"
	$(INSTALL) -m644 tcc-doc.html $(top_srcdir)/win32/tcc-win32.txt "$(tccdir)/doc"
	$(INSTALL) -m644 $(top_srcdir)/libtcc.h $(LIBTCC_EXTRA) "$(tccdir)/libtcc"
	$(INSTALL) -m644 $(LIBTCC) "$(tccdir)"
ifdef CONFIG_CROSS
	mkdir -p "$(tccdir)/lib/32"
	mkdir -p "$(tccdir)/lib/64"
	-$(INSTALL) -m644 lib/i386-win/libtcc1.a "$(tccdir)/lib/32"
	-$(INSTALL) -m644 lib/x86_64-win/libtcc1.a "$(tccdir)/lib/64"
endif

uninstall:
	rm -rfv "$(tccdir)/*"
endif

# documentation and man page
tcc-doc.html: tcc-doc.texi
	-makeinfo --no-split --html --number-sections -o $@ $<

tcc.1: tcc-doc.texi
	-$(top_srcdir)/texi2pod.pl $< tcc.pod
	-pod2man --section=1 --center="Tiny C Compiler" --release=`cat $(top_srcdir)/VERSION` tcc.pod > $@

tcc-doc.info: tcc-doc.texi
	-makeinfo $<

# in tests subdir
export LIBTCC1

%est:
	$(MAKE) -C tests $@ 'PROGS_CROSS=$(PROGS_CROSS)'

clean:
	rm -vf $(PROGS) tcc_p$(EXESUF) tcc.pod *~ *.o *.a *.so* *.out *.log \
		*.exe a.out tags TAGS libtcc_test$(EXESUF) tcc$(EXESUF)
	-rm -r $(ARCH) arm64
ifeq ($(HOST_OS),Linux)
	-rm -r ./C:
endif
	-rm *-tcc$(EXESUF)
	$(MAKE) -C tests $@
ifneq ($(LIBTCC1),)
	$(MAKE) -C lib $@
endif

distclean: clean
	rm -vf config.h config.mak config.texi tcc.1 tcc-doc.info tcc-doc.html

config.mak:
	@echo "Please run ./configure."
	@exit 1

tags:
	ctags $(top_srcdir)/*.[ch] $(top_srcdir)/include/*.h $(top_srcdir)/lib/*.[chS]

TAGS:
	ctags -e $(top_srcdir)/*.[ch] $(top_srcdir)/include/*.h $(top_srcdir)/lib/*.[chS]

# create release tarball from *current* git branch (including tcc-doc.html
# and converting two files to CRLF)
TCC-VERSION := tcc-$(shell cat $(top_srcdir)/VERSION)
tar:    tcc-doc.html
	mkdir $(TCC-VERSION)
	( cd $(TCC-VERSION) && git --git-dir ../.git checkout -f )
	cp tcc-doc.html $(TCC-VERSION)
	for f in tcc-win32.txt build-tcc.bat ; do \
	    cat win32/$$f | sed 's,\(.*\),\1\r,g' > $(TCC-VERSION)/win32/$$f ; \
	done
	tar cjf $(TCC-VERSION).tar.bz2 $(TCC-VERSION)
	rm -rf $(TCC-VERSION)
	git reset

.PHONY: all clean tar tags TAGS distclean install uninstall FORCE

endif # ifeq ($(TOP),.)
