#
# Tiny C Compiler Makefile
#

TOP ?= .
include $(TOP)/config.mak
VPATH = $(TOPSRC)
CFLAGS += -I$(TOP)
CFLAGS += $(CPPFLAGS)

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
 ifneq ($(DISABLE_STATIC),no)
  LIBTCC = libtcc.dll
  LIBTCCDEF = libtcc.def
 endif
else
 LIBS=-lm
 ifndef CONFIG_NOLDL
  LIBS+=-ldl
 endif
 # make libtcc as static or dynamic library?
 ifeq ($(DISABLE_STATIC),yes)
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
NATIVE_DEFINES_$(CONFIG_x86_64) += -DTCC_TARGET_X86_64
NATIVE_DEFINES_$(CONFIG_WIN32) += -DTCC_TARGET_PE
NATIVE_DEFINES_$(CONFIG_uClibc) += -DTCC_UCLIBC
NATIVE_DEFINES_$(CONFIG_arm) += -DTCC_TARGET_ARM
NATIVE_DEFINES_$(CONFIG_arm_eabihf) += -DTCC_ARM_EABI -DTCC_ARM_HARDFLOAT
NATIVE_DEFINES_$(CONFIG_arm_eabi) += -DTCC_ARM_EABI
NATIVE_DEFINES_$(CONFIG_arm_vfp) += -DTCC_ARM_VFP
NATIVE_DEFINES_$(CONFIG_arm64) += -DTCC_TARGET_ARM64
NATIVE_DEFINES += $(NATIVE_DEFINES_yes)

# --------------------------------------------------------------------------
# running top Makefile
ifeq ($(TOP),.)

CORE_FILES = tcc.c tcctools.c libtcc.c tccpp.c tccgen.c tccelf.c tccasm.c tccrun.c
CORE_FILES += tcc.h config.h libtcc.h tcctok.h
i386_FILES = $(CORE_FILES) i386-gen.c i386-link.c i386-asm.c i386-asm.h i386-tok.h
i386-win32_FILES = $(i386_FILES) tccpe.c
x86_64_FILES = $(CORE_FILES) x86_64-gen.c x86_64-link.c i386-asm.c x86_64-asm.h
x86_64-win32_FILES = $(x86_64_FILES) tccpe.c
arm_FILES = $(CORE_FILES) arm-gen.c arm-link.c
arm-wince_FILES = $(arm_FILES) tccpe.c
arm64_FILES = $(CORE_FILES) arm64-gen.c arm64-link.c
c67_FILES = $(CORE_FILES) c67-gen.c c67-link.c tcccoff.c

CFGWIN = $(if $(CONFIG_WIN32),-win)
NATIVE_TARGET = $(ARCH)$(if $(CONFIG_WIN32),-win$(if $(eq $(ARCH),arm),ce,32))
NATIVE_FILES = $($(NATIVE_TARGET)_FILES)

PROGS = tcc$(EXESUF)
TCCLIBS = $(LIBTCC1) $(LIBTCC) $(LIBTCCDEF)
TCCDOCS = tcc.1 tcc-doc.html tcc-doc.info

all: $(PROGS) $(TCCLIBS) $(TCCDOCS)

# Host Tiny C Compiler
tcc$(EXESUF): tcc.o $(LIBTCC)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LIBS) $(LINK_LIBTCC)

# profiling version
tcc_p$(EXESUF): $(NATIVE_FILES)
	$(CC) -o $@ $< -DONE_SOURCE $(NATIVE_DEFINES) $(CPPFLAGS_P) $(CFLAGS_P) $(LIBS_P) $(LDFLAGS_P)

# libtcc generation and test
ifndef ONE_SOURCE
LIBTCC_OBJ = $(filter-out tcc.o tcctools.o,$(patsubst %.c,%.o,$(filter %.c,$(NATIVE_FILES))))
LIBTCC_INC = $(filter %.h,$(CORE_FILES)) $(filter-out $(CORE_FILES) i386-asm.c,$(NATIVE_FILES))
else
LIBTCC_OBJ = libtcc.o
LIBTCC_INC = $(NATIVE_FILES)
libtcc.o : NATIVE_DEFINES += -DONE_SOURCE
endif

tcc.o : tcctools.c

$(LIBTCC_OBJ) tcc.o : %.o : %.c $(LIBTCC_INC)
	$(CC) -o $@ -c $< $(NATIVE_DEFINES) $(CFLAGS)

libtcc.a: $(LIBTCC_OBJ)
	$(AR) rcs $@ $^

libtcc.so: $(LIBTCC_OBJ)
	$(CC) -shared -Wl,-soname,$@ -o $@ $^ $(LDFLAGS)

libtcc.so: CFLAGS+=-fPIC

# windows : libtcc.dll
libtcc.dll : $(LIBTCC_OBJ)
	$(CC) -shared $(LIBTCC_OBJ) -o $@ $(LDFLAGS)

libtcc.def : libtcc.dll tcc$(EXESUF)
	./tcc$(EXESUF) -impdef $< -o $@

libtcc.dll : NATIVE_DEFINES += -DLIBTCC_AS_DLL

# TinyCC runtime libraries
libtcc1.a : FORCE tcc$(EXESUF)
	$(MAKE) -C lib TARGET=$(NATIVE_TARGET)

FORCE:
install: install-native$(CFGWIN)
uninstall: uninstall-native$(CFGWIN)

# cross compilers
# --------------------------------------------------------------------------
ifdef CONFIG_CROSS

I386_CROSS = i386-tcc$(EXESUF)
X64_CROSS = x86_64-tcc$(EXESUF)
WIN32_CROSS = i386-win32-tcc$(EXESUF)
WIN64_CROSS = x86_64-win32-tcc$(EXESUF)
WINCE_CROSS = arm-wince-tcc$(EXESUF)
C67_CROSS = c67-tcc$(EXESUF)
ARM64_CROSS = arm64-tcc$(EXESUF)
ARM_FPA_CROSS = arm-fpa-tcc$(EXESUF)
ARM_FPA_LD_CROSS = arm-fpa-ld-tcc$(EXESUF)
ARM_VFP_CROSS = arm-vfp-tcc$(EXESUF)
ARM_EABI_CROSS = arm-eabi-tcc$(EXESUF)
ARM_EABIHF_CROSS = arm-eabihf-tcc$(EXESUF)
# ARM_CROSS = $(ARM_FPA_CROSS) $(ARM_FPA_LD_CROSS) $(ARM_VFP_CROSS) $(ARM_EABI_CROSS) $(ARM_EABIHF_CROSS)
ARM_CROSS = $(ARM_EABIHF_CROSS)

$(I386_CROSS)   : $(i386_FILES)
$(X64_CROSS)    : $(x86_64_FILES)
$(WIN32_CROSS)  : $(i386-win32_FILES)
$(WIN64_CROSS)  : $(x86_64-win32_FILES)
$(WINCE_CROSS)  : $(arm-wince_FILES)
$(ARM_CROSS)    : $(arm_FILES)
$(ARM64_CROSS)  : $(arm64_FILES)
$(C67_CROSS)    : $(c67_FILES)

$(I386_CROSS)   : T = i386
$(X64_CROSS)    : T = x86_64
$(WIN32_CROSS)  : T = i386-win32
$(WIN64_CROSS)  : T = x86_64-win32
$(WINCE_CROSS)  : T = arm-wince
$(ARM64_CROSS)  : T = arm64
$(C67_CROSS)    : T = c67
$(ARM_EABIHF_CROSS) : T = arm-eabihf

$(I386_CROSS)   : DEFINES += -DTCC_TARGET_I386
$(X64_CROSS)    : DEFINES += -DTCC_TARGET_X86_64
$(WIN32_CROSS)  : DEFINES += -DTCC_TARGET_PE -DTCC_TARGET_I386
$(WIN64_CROSS)  : DEFINES += -DTCC_TARGET_PE -DTCC_TARGET_X86_64
$(WINCE_CROSS)  : DEFINES += -DTCC_TARGET_PE -DTCC_TARGET_ARM -DTCC_ARM_EABI -DTCC_ARM_VFP -DTCC_ARM_HARDFLOAT
$(ARM64_CROSS)  : DEFINES += -DTCC_TARGET_ARM64
$(C67_CROSS)    : DEFINES += -DTCC_TARGET_C67 -w # disable warnigs
$(ARM_CROSS)    : DEFINES += -DTCC_TARGET_ARM
$(ARM_FPA_LD_CROSS) : DEFINES += -DLDOUBLE_SIZE=12
$(ARM_VFP_CROSS)    : DEFINES += -DTCC_ARM_VFP
$(ARM_EABI_CROSS)   : DEFINES += -DTCC_ARM_VFP -DTCC_ARM_EABI
$(ARM_EABIHF_CROSS) : DEFINES += -DTCC_ARM_VFP -DTCC_ARM_EABI -DTCC_ARM_HARDFLOAT

DEFINES += $(DEF-$T) $(DEF-all)
DEFINES += $(if $(ROOT-$T),-DCONFIG_SYSROOT="\"$(ROOT-$T)\"")
DEFINES += $(if $(CRT-$T),-DCONFIG_TCC_CRTPREFIX="\"$(CRT-$T)\"")
DEFINES += $(if $(LIB-$T),-DCONFIG_TCC_LIBPATHS="\"$(LIB-$T)\"")
DEFINES += $(if $(INC-$T),-DCONFIG_TCC_SYSINCLUDEPATHS="\"$(INC-$T)\"")
DEFINES += $(DEF-$(or $(findstring win,$T),unx))

ifeq ($(CONFIG_WIN32),yes)
DEF-win += -DTCC_LIBTCC1="\"libtcc1-$T.a\""
DEF-unx += -DTCC_LIBTCC1="\"lib/libtcc1-$T.a\""
else
DEF-all += -DTCC_LIBTCC1="\"libtcc1-$T.a\""
DEF-win += -DCONFIG_TCCDIR="\"$(tccdir)/win32\""
endif

DEF-$(NATIVE_TARGET) += $(NATIVE_DEFINES)

# For a (non-windows-) cross compiler to really work
# you need to create a file 'config-cross.mak'
# ----------------------------------------------------
# Example config-cross.mak:
#
# windows -> i386-linux cross-compiler
# (it expects the linux files in <prefix>/i386-linux)
#
# ROOT-i386 = {B}/i386-linux
# CRT-i386 = $(ROOT-i386)/usr/lib
# LIB-i386 = $(ROOT-i386)/lib:$(ROOT-i386)/usr/lib
# INC-i386 = {B}/lib/include:$(ROOT-i386)/usr/include
# DEF-i386 += -D__linux__
#
# ----------------------------------------------------

-include config-cross.mak

# cross tcc to build
PROGS_CROSS = $(I386_CROSS) $(X64_CROSS) $(WIN32_CROSS) $(WIN64_CROSS)
PROGS_CROSS += $(ARM_CROSS) $(ARM64_CROSS) $(WINCE_CROSS) $(C67_CROSS)

# cross libtcc1.a targets to build
LIBTCC1_TARGETS = i386 x86_64 i386-win32 x86_64-win32 arm-eabihf arm64 arm-wince

all : $(foreach l,$(LIBTCC1_TARGETS),lib/libtcc1-$l.a)
all : $(PROGS_CROSS)

# Cross Tiny C Compilers
%-tcc$(EXESUF): tcc.c
	$(CC) -o $@ $< -DONE_SOURCE $(DEFINES) $(CFLAGS) $(LIBS) $(LDFLAGS)

# Cross libtcc1.a
lib/libtcc1-%.a : FORCE %-tcc$(EXESUF)
	$(MAKE) -C lib TARGET=$* CROSS=yes

# install cross progs & libs
install-cross:
	$(INSTALLBIN) -m755 $(PROGS_CROSS) "$(bindir)"
	mkdir -p "$(tccdir)/win32/include"
	cp -r $(TOPSRC)/include/. "$(tccdir)/win32/include"
	cp -r $(TOPSRC)/win32/include/. "$(tccdir)/win32/include"
	mkdir -p "$(tccdir)/win32/lib"
	$(INSTALL) -m644 $(TOPSRC)/win32/lib/*.def "$(tccdir)/win32/lib"
install-cross-lib-%:
	$(INSTALL) -m644 lib/libtcc1-$*.a "$(tccdir)$(libtcc1dir)"
install-cross-lib-%-win32 install-cross-lib-%-wince: libtcc1dir = /win32/lib

# install cross progs & libs on windows
install-cross-win:
	$(INSTALLBIN) -m755 $(PROGS_CROSS) "$(tccdir)"
	mkdir -p "$(tccdir)/lib/include"
	$(INSTALL) -m644 $(TOPSRC)/include/*.h $(TOPSRC)/tcclib.h "$(tccdir)/lib/include"
install-cross-win-lib-%:
	$(INSTALL) -m644 lib/libtcc1-$*.a "$(tccdir)/lib"

install: install-cross$(CFGWIN)
install: $(foreach t,$(LIBTCC1_TARGETS),install-cross$(CFGWIN)-lib-$t)

endif # def CONFIG_CROSS
# --------------------------------------------------------------------------

# install
INSTALL = install
INSTALLBIN = install $(STRIP_$(STRIP_BINARIES))
STRIP_yes = -s

install-strip: install
install-strip: STRIP_BINARIES = yes

install-native:
	mkdir -p "$(bindir)"
	$(INSTALLBIN) -m755 $(PROGS) "$(bindir)"
	mkdir -p "$(tccdir)"
	$(if $(LIBTCC1),$(INSTALL) -m644 $(LIBTCC1) "$(tccdir)")
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

uninstall-native:
	rm -fv $(foreach P,$(PROGS),"$(bindir)/$P")
	rm -fv "$(libdir)/$(LIBTCC)" "$(includedir)/libtcc.h"
	rm -fv "$(mandir)/man1/tcc.1" "$(infodir)/tcc-doc.info"
	rm -fv "$(docdir)/tcc-doc.html"
	rm -rv "$(tccdir)"

install-native-win:
	mkdir -p "$(tccdir)"
	mkdir -p "$(tccdir)/lib"
	mkdir -p "$(tccdir)/include"
	mkdir -p "$(tccdir)/examples"
	mkdir -p "$(tccdir)/doc"
	mkdir -p "$(tccdir)/libtcc"
	$(INSTALLBIN) -m755 $(PROGS) $(subst libtcc.a,,$(LIBTCC)) "$(tccdir)"
	$(INSTALL) -m644 libtcc1.a $(TOPSRC)/win32/lib/*.def "$(tccdir)/lib"
	cp -r $(TOPSRC)/win32/include/. "$(tccdir)/include"
	cp -r $(TOPSRC)/win32/examples/. "$(tccdir)/examples"
	cp $(TOPSRC)/tests/libtcc_test.c "$(tccdir)/examples"
	$(INSTALL) -m644 $(TOPSRC)/include/*.h $(TOPSRC)/tcclib.h "$(tccdir)/include"
	$(INSTALL) -m644 $(TOPSRC)/libtcc.h $(subst .dll,.def,$(LIBTCC)) "$(tccdir)/libtcc"
	-$(INSTALL) -m644 $(TOPSRC)/win32/tcc-win32.txt tcc-doc.html "$(tccdir)/doc"

uninstall-native-win:
	rm -rfv "$(tccdir)/"*

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
	rm -f $(PROGS) $(PROGS_CROSS) tcc_p$(EXESUF) tcc.pod \
	    *~ *.o *.a *.so* *.out *.log lib*.def *.exe *.dll a.out \
	    tags TAGS libtcc_test$(EXESUF)
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
