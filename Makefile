#
# Tiny C Compiler Makefile
#

TOP ?= .
include $(TOP)/config.mak
VPATH = $(TOPSRC)
CFLAGS += -I$(TOP)

ifeq (-$(findstring gcc,$(CC))-,-gcc-)
 ifeq (-$(GCC_MAJOR)-$(findstring $(GCC_MINOR),56789)-,-4--)
  CFLAGS += -D_FORTIFY_SOURCE=0
 endif
else
 ifeq (-$(findstring clang,$(CC))-,-clang-)
  # make clang accept gnuisms in libtcc1.c
  CFLAGS+=-fheinous-gnu-extensions
 endif
endif

LIBTCC = libtcc.a
LIBTCC1 = libtcc1.a
LINK_LIBTCC =
LIBS =

ifdef CONFIG_WIN32
 LIBTCC = libtcc.dll
else
 LIBS=-lm
 ifndef CONFIG_NOLDL
  LIBS+=-ldl
 endif
 # make libtcc as static or dynamic library?
 ifdef DISABLE_STATIC
  LIBTCC=libtcc.so
  ifndef DISABLE_RPATH
   LINK_LIBTCC += -Wl,-rpath,"$(libdir)"
   export LD_LIBRARY_PATH := $(CURDIR)/$(TOP)
  endif
 endif
endif

ifeq ($(TARGETOS),Darwin)
 CFLAGS += -Wl,-flat_namespace,-undefined,warning
 export MACOSX_DEPLOYMENT_TARGET:=10.2
endif

CFLAGS_P = $(CFLAGS) -pg -static -DCONFIG_TCC_STATIC -DTCC_PROFILE
LIBS_P= $(LIBS)
LDFLAGS_P = $(LDFLAGS)

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
I386_CROSS = i386-tcc$(EXESUF)
WIN32_CROSS = i386-win32-tcc$(EXESUF)
WIN64_CROSS = x86_64-win32-tcc$(EXESUF)
WINCE_CROSS = arm-wince-tcc$(EXESUF)
X64_CROSS = x86_64-tcc$(EXESUF)
ARM_FPA_CROSS = arm-fpa-tcc$(EXESUF)
ARM_FPA_LD_CROSS = arm-fpa-ld-tcc$(EXESUF)
ARM_VFP_CROSS = arm-vfp-tcc$(EXESUF)
ARM_EABI_CROSS = arm-eabi-tcc$(EXESUF)
ARM_EABIHF_CROSS = arm-eabihf-tcc$(EXESUF)
ARM_CROSS = $(ARM_FPA_CROSS) $(ARM_FPA_LD_CROSS) $(ARM_VFP_CROSS) $(ARM_EABI_CROSS)
ARM64_CROSS = arm64-tcc$(EXESUF)
C67_CROSS = c67-tcc$(EXESUF)

CORE_FILES = tcc.c libtcc.c tccpp.c tccgen.c tccelf.c tccasm.c tccrun.c
CORE_FILES += tcc.h config.h libtcc.h tcctok.h
I386_FILES = $(CORE_FILES) i386-gen.c i386-link.c i386-asm.c i386-asm.h i386-tok.h
WIN32_FILES = $(CORE_FILES) i386-gen.c i386-link.c i386-asm.c i386-asm.h i386-tok.h tccpe.c
WIN64_FILES = $(CORE_FILES) x86_64-gen.c x86_64-link.c i386-asm.c x86_64-asm.h tccpe.c
WINCE_FILES = $(CORE_FILES) arm-gen.c arm-link.c tccpe.c
X86_64_FILES = $(CORE_FILES) x86_64-gen.c x86_64-link.c i386-asm.c x86_64-asm.h
ARM_FILES = $(CORE_FILES) arm-gen.c arm-link.c
ARM64_FILES = $(CORE_FILES) arm64-gen.c arm64-link.c
C67_FILES = $(CORE_FILES) c67-gen.c c67-link.c tcccoff.c

ifdef CONFIG_WIN32
 PROGS+=tiny_impdef$(EXESUF) tiny_libmaker$(EXESUF) $(LIBTCC)
 ifeq ($(ARCH),x86-64)
  NATIVE_FILES=$(WIN64_FILES)
  PROGS_CROSS=$(WIN32_CROSS) $(X64_CROSS) $(ARM_CROSS) $(ARM64_CROSS) $(C67_CROSS) $(WINCE_CROSS)
  LIBTCC1_CROSS=lib/i386-win32/libtcc1.a
 else
  NATIVE_FILES=$(WIN32_FILES)
  PROGS_CROSS=$(WIN64_CROSS) $(X64_CROSS) $(ARM_CROSS) $(ARM64_CROSS) $(C67_CROSS) $(WINCE_CROSS)
  LIBTCC1_CROSS=lib/x86_64-win32/libtcc1.a
 endif

else ifeq ($(ARCH),i386)
NATIVE_FILES=$(I386_FILES)
PROGS_CROSS=$(X64_CROSS) $(WIN32_CROSS) $(WIN64_CROSS) $(ARM_CROSS) $(ARM64_CROSS) $(C67_CROSS) $(WINCE_CROSS)
LIBTCC1_CROSS=lib/i386-win32/libtcc1.a lib/x86_64-win32/libtcc1.a

else ifeq ($(ARCH),x86-64)
NATIVE_FILES=$(X86_64_FILES)
PROGS_CROSS=$(I386_CROSS) $(WIN32_CROSS) $(WIN64_CROSS) $(ARM_CROSS) $(ARM64_CROSS) $(C67_CROSS) $(WINCE_CROSS)
LIBTCC1_CROSS=lib/i386-win32/libtcc1.a lib/x86_64-win32/libtcc1.a

else ifeq ($(ARCH),arm)
NATIVE_FILES=$(ARM_FILES)
PROGS_CROSS=$(I386_CROSS) $(X64_CROSS) $(WIN32_CROSS) $(WIN64_CROSS) $(ARM64_CROSS) $(C67_CROSS) $(WINCE_CROSS)

else ifeq ($(ARCH),arm64)
NATIVE_FILES=$(ARM64_FILES)
PROGS_CROSS=$(I386_CROSS) $(X64_CROSS) $(WIN32_CROSS) $(WIN64_CROSS) $(ARM_CROSS) $(C67_CROSS) $(WINCE_CROSS)
endif

ifeq ($(TARGETOS),Darwin)
 PROGS += tiny_libmaker$(EXESUF)
endif

TCCLIBS = $(LIBTCC1) $(LIBTCC)
TCCDOCS = tcc.1 tcc-doc.html tcc-doc.info

ifdef CONFIG_CROSS
 PROGS += $(PROGS_CROSS)
 TCCLIBS += $(LIBTCC1_CROSS)
endif

all: $(PROGS) $(TCCLIBS) $(TCCDOCS)

# Host Tiny C Compiler
tcc$(EXESUF): tcc.o $(LIBTCC)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LIBS) $(LINK_LIBTCC)

# Cross Tiny C Compilers
%-tcc$(EXESUF): tcc.c
	$(CC) -o $@ $< -DONE_SOURCE $(DEFINES) $(CFLAGS) $(LIBS) $(LDFLAGS)

# profiling version
tcc_p$(EXESUF): $(NATIVE_FILES)
	$(CC) -o $@ $< -DONE_SOURCE $(NATIVE_DEFINES) $(CPPFLAGS_P) $(CFLAGS_P) $(LIBS_P) $(LDFLAGS_P)

$(I386_CROSS): DEFINES = -DTCC_TARGET_I386 \
    -DCONFIG_TCCDIR="\"$(tccdir)/i386\""
$(X64_CROSS) : DEFINES = -DTCC_TARGET_X86_64
    -DCONFIG_TCCDIR="\"$(tccdir)/x86_64\""
$(WIN32_CROSS) : DEFINES = -DTCC_TARGET_I386 -DTCC_TARGET_PE \
    -DCONFIG_TCCDIR="\"$(tccdir)/win32\"" \
    -DCONFIG_TCC_LIBPATHS="\"{B}/lib/32;{B}/lib\""
$(WIN64_CROSS) : DEFINES = -DTCC_TARGET_X86_64 -DTCC_TARGET_PE \
    -DCONFIG_TCCDIR="\"$(tccdir)/win32\"" \
    -DCONFIG_TCC_LIBPATHS="\"{B}/lib/64;{B}/lib\""
$(WINCE_CROSS): DEFINES = -DTCC_TARGET_ARM -DTCC_TARGET_PE
$(C67_CROSS): DEFINES = -DTCC_TARGET_C67 -w # disable warnigs
$(ARM_FPA_CROSS): DEFINES = -DTCC_TARGET_ARM
$(ARM_FPA_LD_CROSS)$(EXESUF): DEFINES = -DTCC_TARGET_ARM -DLDOUBLE_SIZE=12
$(ARM_VFP_CROSS): DEFINES = -DTCC_TARGET_ARM -DTCC_ARM_VFP
$(ARM_EABI_CROSS): DEFINES = -DTCC_TARGET_ARM -DTCC_ARM_EABI -DTCC_ARM_VFP
$(ARM64_CROSS): DEFINES = -DTCC_TARGET_ARM64

$(I386_CROSS) : $(I386_FILES)
$(X64_CROSS)  : $(X86_64_FILES)
$(WIN32_CROSS) : $(WIN32_FILES)
$(WIN64_CROSS) : $(WIN64_FILES)
$(WINCE_CROSS) : $(WINCE_FILES)
$(C67_CROSS)  : $(C67_FILES)
$(ARM_FPA_CROSS) $(ARM_FPA_LD_CROSS) $(ARM_VFP_CROSS) $(ARM_EABI_CROSS): $(ARM_FILES)
$(ARM64_CROSS): $(ARM64_FILES)

# libtcc generation and test
ifndef ONE_SOURCE
LIBTCC_OBJ = $(filter-out tcc.o,$(patsubst %.c,%.o,$(filter %.c,$(NATIVE_FILES))))
LIBTCC_INC = $(filter %.h,$(CORE_FILES)) $(filter-out $(CORE_FILES) i386-asm.c,$(NATIVE_FILES))
else
LIBTCC_OBJ = libtcc.o
LIBTCC_INC = $(NATIVE_FILES)
libtcc.o : NATIVE_DEFINES += -DONE_SOURCE
endif

$(LIBTCC_OBJ) tcc.o : %.o : %.c $(LIBTCC_INC)
	$(CC) -o $@ -c $< $(NATIVE_DEFINES) $(CFLAGS)

libtcc.a: $(LIBTCC_OBJ)
	$(AR) rcs $@ $^

libtcc.so: $(LIBTCC_OBJ)
	$(CC) -shared -Wl,-soname,$@ -o $@ $^ $(LDFLAGS)

libtcc.so: CFLAGS+=-fPIC

# windows : libtcc.dll
libtcc.dll : $(LIBTCC_OBJ) tiny_impdef$(EXESUF)
	$(CC) -shared $(LIBTCC_OBJ) -o $@ $(LDFLAGS)
	./tiny_impdef $@

libtcc.dll : NATIVE_DEFINES += -DLIBTCC_AS_DLL

# windows : utilities
tiny_%$(EXESUF): $(TOPSRC)/win32/tools/tiny_%.c
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS) $(NATIVE_DEFINES)

ifneq ($(LIBTCC1),)
# TinyCC runtime libraries
$(LIBTCC1) : FORCE $(PROGS)
	$(MAKE) -C lib native
endif

lib/%/libtcc1.a : FORCE $(PROGS_CROSS)
	$(MAKE) -C lib cross TARGET=$*

FORCE:

# install
INSTALL = install
INSTALLBIN = install $(STRIP_$(STRIP_BINARIES))
STRIP_yes = -s

install-strip: install
install-strip: STRIP_BINARIES = yes

ifndef CONFIG_WIN32
install:
	mkdir -p "$(bindir)"
	$(INSTALLBIN) -m755 $(PROGS) "$(bindir)"
	mkdir -p "$(tccdir)"
ifneq ($(LIBTCC1),)
	$(INSTALL) -m644 $(LIBTCC1) "$(tccdir)"
endif
	mkdir -p "$(tccdir)/include"
	$(INSTALL) -m644 $(TOPSRC)/include/*.h $(TOPSRC)/tcclib.h "$(tccdir)/include"
	mkdir -p "$(libdir)"
	$(INSTALL) -m644 $(LIBTCC) "$(libdir)"
	mkdir -p "$(includedir)"
	$(INSTALL) -m644 $(TOPSRC)/libtcc.h "$(includedir)"

	mkdir -p "$(mandir)/man1"
	-$(INSTALL) -m644 tcc.1 "$(mandir)/man1"
	mkdir -p "$(infodir)"
	-$(INSTALL) -m644 tcc-doc.info "$(infodir)"
	mkdir -p "$(docdir)"
	-$(INSTALL) -m644 tcc-doc.html "$(docdir)"
ifdef CONFIG_CROSS
	mkdir -p "$(tccdir)/win32/lib/32"
	mkdir -p "$(tccdir)/win32/lib/64"
	$(INSTALL) -m644 $(TOPSRC)/win32/lib/*.def "$(tccdir)/win32/lib"
	-$(INSTALL) -m644 lib/i386-win32/libtcc1.a "$(tccdir)/win32/lib/32"
	-$(INSTALL) -m644 lib/x86_64-win32/libtcc1.a "$(tccdir)/win32/lib/64"
	cp -r $(TOPSRC)/include/. "$(tccdir)/win32/include"
	cp -r $(TOPSRC)/win32/include/. "$(tccdir)/win32/include"
endif

uninstall:
	rm -fv $(foreach P,$(PROGS),"$(bindir)/$P")
	rm -fv "$(libdir)/$(LIBTCC)" "$(includedir)/libtcc.h"
	rm -fv "$(mandir)/man1/tcc.1" "$(infodir)/tcc-doc.info"
	rm -fv "$(docdir)/tcc-doc.html"
	rm -rv "$(tccdir)"
else
# on windows
install:
	mkdir -p "$(tccdir)"
	mkdir -p "$(tccdir)/lib"
	mkdir -p "$(tccdir)/include"
	mkdir -p "$(tccdir)/examples"
	mkdir -p "$(tccdir)/doc"
	mkdir -p "$(tccdir)/libtcc"
	$(INSTALLBIN) -m755 $(PROGS) "$(tccdir)"
	$(INSTALL) -m644 libtcc1.a $(TOPSRC)/win32/lib/*.def "$(tccdir)/lib"
	cp -r $(TOPSRC)/win32/include/. "$(tccdir)/include"
	cp -r $(TOPSRC)/win32/examples/. "$(tccdir)/examples"
	cp $(TOPSRC)/tests/libtcc_test.c "$(tccdir)/examples"
	$(INSTALL) -m644 $(TOPSRC)/include/*.h $(TOPSRC)/tcclib.h "$(tccdir)/include"
	$(INSTALL) -m644 tcc-doc.html $(TOPSRC)/win32/tcc-win32.txt "$(tccdir)/doc"
	$(INSTALL) -m644 $(TOPSRC)/libtcc.h libtcc.def "$(tccdir)/libtcc"
ifdef CONFIG_CROSS
	mkdir -p "$(tccdir)/lib/32"
	mkdir -p "$(tccdir)/lib/64"
	-$(INSTALL) -m644 lib/i386-win32/libtcc1.a "$(tccdir)/lib/32"
	-$(INSTALL) -m644 lib/x86_64-win32/libtcc1.a "$(tccdir)/lib/64"
endif

uninstall:
	rm -rfv "$(tccdir)/"*
endif

# documentation and man page
tcc-doc.html: tcc-doc.texi
	-makeinfo --no-split --html --number-sections -o $@ $<

tcc.1: tcc-doc.texi
	-$(TOPSRC)/texi2pod.pl $< tcc.pod
	-pod2man --section=1 --center="Tiny C Compiler" --release="$(VERSION)" tcc.pod > $@

tcc-doc.info: tcc-doc.texi
	-makeinfo $<

# in tests subdir
test:
	$(MAKE) -C tests

clean:
	rm -f $(PROGS) tcc_p$(EXESUF) tcc.pod *~ *.o *.a *.so* *.out *.log \
	    lib*.def *.exe *.dll a.out tags TAGS libtcc_test$(EXESUF) tcc$(EXESUF)
	$(MAKE) -C tests $@
	$(MAKE) -C lib $@

distclean: clean
	rm -f config.h config.mak config.texi tcc.1 tcc-doc.info tcc-doc.html

config.mak:
	@echo "Please run ./configure."
	@exit 1

TAGFILES = *.[ch] include/*.h lib/*.[chS]
tags : ; ctags $(TAGFILES)
TAGS : ; etags $(TAGFILES)

# create release tarball from *current* git branch (including tcc-doc.html
# and converting two files to CRLF)
TCC-VERSION = $(VERSION)
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

.PHONY: all clean test tar tags TAGS distclean install uninstall FORCE

endif # ifeq ($(TOP),.)
