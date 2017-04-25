# --------------------------------------------------------------------------
#
# Tiny C Compiler Makefile
#

ifndef TOP
 TOP = .
 INCLUDED = no
endif

include $(TOP)/config.mak

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
CFLAGS += -I$(TOP)
CFLAGS += $(CPPFLAGS)
VPATH = $(TOPSRC)

ifdef CONFIG_WIN32
 ifneq ($(DISABLE_STATIC),no)
  LIBTCC = libtcc$(DLLSUF)
  LIBTCCDEF = libtcc.def
 endif
 CFGWIN = -win
 NATIVE_TARGET = $(ARCH)-win$(if $(findstring arm,$(ARCH)),ce,32)
else
 LIBS=-lm
 ifndef CONFIG_NOLDL
  LIBS+=-ldl
 endif
 # make libtcc as static or dynamic library?
 ifeq ($(DISABLE_STATIC),yes)
  LIBTCC=libtcc$(DLLSUF)
  export LD_LIBRARY_PATH := $(CURDIR)/$(TOP)
  ifndef DISABLE_RPATH
   LINK_LIBTCC += -Wl,-rpath,"$(libdir)"
  endif
 endif
 CFGWIN =-unx
 NATIVE_TARGET = $(ARCH)
 ifdef CONFIG_OSX
  NATIVE_TARGET = $(ARCH)-osx
  LDFLAGS += -flat_namespace -undefined warning
  export MACOSX_DEPLOYMENT_TARGET := 10.2
 endif
endif

# run local version of tcc with local libraries and includes
TCCFLAGS-unx = -B$(TOP) -I$(TOPSRC)/include -I$(TOPSRC) -I$(TOP)
TCCFLAGS-win = -B$(TOPSRC)/win32 -I$(TOPSRC)/include -I$(TOPSRC) -I$(TOP) -L$(TOP)
TCCFLAGS = $(TCCFLAGS$(CFGWIN))
TCC = $(TOP)/tcc$(EXESUF) $(TCCFLAGS)
ifdef CONFIG_OSX
 TCCFLAGS += -D_ANSI_SOURCE
endif

CFLAGS_P = $(CFLAGS) -pg -static -DCONFIG_TCC_STATIC -DTCC_PROFILE
LIBS_P = $(LIBS)
LDFLAGS_P = $(LDFLAGS)

CONFIG_$(ARCH) = yes
NATIVE_DEFINES_$(CONFIG_i386) += -DTCC_TARGET_I386
NATIVE_DEFINES_$(CONFIG_x86_64) += -DTCC_TARGET_X86_64
NATIVE_DEFINES_$(CONFIG_WIN32) += -DTCC_TARGET_PE
NATIVE_DEFINES_$(CONFIG_OSX) += -DTCC_TARGET_MACHO
NATIVE_DEFINES_$(CONFIG_uClibc) += -DTCC_UCLIBC
NATIVE_DEFINES_$(CONFIG_musl) += -DTCC_MUSL
NATIVE_DEFINES_$(CONFIG_arm) += -DTCC_TARGET_ARM
NATIVE_DEFINES_$(CONFIG_arm_eabihf) += -DTCC_ARM_EABI -DTCC_ARM_HARDFLOAT
NATIVE_DEFINES_$(CONFIG_arm_eabi) += -DTCC_ARM_EABI
NATIVE_DEFINES_$(CONFIG_arm_vfp) += -DTCC_ARM_VFP
NATIVE_DEFINES_$(CONFIG_arm64) += -DTCC_TARGET_ARM64
NATIVE_DEFINES += $(NATIVE_DEFINES_yes)

ifeq ($(INCLUDED),no)
# --------------------------------------------------------------------------
# running top Makefile

PROGS = tcc$(EXESUF)
TCCLIBS = $(LIBTCC1) $(LIBTCC) $(LIBTCCDEF)
TCCDOCS = tcc.1 tcc-doc.html tcc-doc.info

all: $(PROGS) $(TCCLIBS) $(TCCDOCS)

# cross compiler targets to build
TCC_X = i386 x86_64 i386-win32 x86_64-win32 x86_64-osx arm arm64 arm-wince c67
# TCC_X += arm-fpa arm-fpa-ld arm-vfp arm-eabi

# cross libtcc1.a targets to build
LIBTCC1_X = i386 x86_64 i386-win32 x86_64-win32 x86_64-osx arm arm64 arm-wince

PROGS_CROSS = $(foreach X,$(TCC_X),$X-tcc$(EXESUF))
LIBTCC1_CROSS = $(foreach X,$(LIBTCC1_X),libtcc1-$X.a)

# build cross compilers & libs
cross: $(LIBTCC1_CROSS) $(PROGS_CROSS)

# build specific cross compiler & lib
cross-%: %-tcc$(EXESUF) libtcc1-%.a ;

install: install$(CFGWIN)
uninstall: uninstall$(CFGWIN)

ifdef CONFIG_CROSS
all : cross
endif

# --------------------------------------------

T = $(or $(CROSS_TARGET),$(NATIVE_TARGET),unknown)
X = $(if $(CROSS_TARGET),$(CROSS_TARGET)-)

DEF-i386        = -DTCC_TARGET_I386
DEF-x86_64      = -DTCC_TARGET_X86_64
DEF-i386-win32  = -DTCC_TARGET_PE -DTCC_TARGET_I386
DEF-x86_64-win32= -DTCC_TARGET_PE -DTCC_TARGET_X86_64
DEF-x86_64-osx  = -DTCC_TARGET_MACHO -DTCC_TARGET_X86_64
DEF-arm-wince   = -DTCC_TARGET_PE -DTCC_TARGET_ARM -DTCC_ARM_EABI -DTCC_ARM_VFP -DTCC_ARM_HARDFLOAT
DEF-arm64       = -DTCC_TARGET_ARM64
DEF-c67         = -DTCC_TARGET_C67 -w # disable warnigs
DEF-arm-fpa     = -DTCC_TARGET_ARM
DEF-arm-fpa-ld  = -DTCC_TARGET_ARM -DLDOUBLE_SIZE=12
DEF-arm-vfp     = -DTCC_TARGET_ARM -DTCC_ARM_VFP
DEF-arm-eabi    = -DTCC_TARGET_ARM -DTCC_ARM_VFP -DTCC_ARM_EABI
DEF-arm-eabihf  = -DTCC_TARGET_ARM -DTCC_ARM_VFP -DTCC_ARM_EABI -DTCC_ARM_HARDFLOAT
DEF-arm         = $(DEF-arm-eabihf)
DEF-$(NATIVE_TARGET) = $(NATIVE_DEFINES)

DEFINES += $(DEF-$T) $(DEF-all)
DEFINES += $(if $(ROOT-$T),-DCONFIG_SYSROOT="\"$(ROOT-$T)\"")
DEFINES += $(if $(CRT-$T),-DCONFIG_TCC_CRTPREFIX="\"$(CRT-$T)\"")
DEFINES += $(if $(LIB-$T),-DCONFIG_TCC_LIBPATHS="\"$(LIB-$T)\"")
DEFINES += $(if $(INC-$T),-DCONFIG_TCC_SYSINCLUDEPATHS="\"$(INC-$T)\"")
DEFINES += $(DEF-$(or $(findstring win,$T),unx))

ifneq ($(X),)
ifeq ($(CONFIG_WIN32),yes)
DEF-win += -DTCC_LIBTCC1="\"libtcc1-$T.a\""
DEF-unx += -DTCC_LIBTCC1="\"lib/libtcc1-$T.a\""
else
DEF-all += -DTCC_LIBTCC1="\"libtcc1-$T.a\""
DEF-win += -DCONFIG_TCCDIR="\"$(tccdir)/win32\""
endif
endif

# include custom cross-compiler configuration (see make help)
-include config-cross.mak

CORE_FILES = tcc.c tcctools.c libtcc.c tccpp.c tccgen.c tccelf.c tccasm.c tccrun.c
CORE_FILES += tcc.h config.h libtcc.h tcctok.h
i386_FILES = $(CORE_FILES) i386-gen.c i386-link.c i386-asm.c i386-asm.h i386-tok.h
i386-win32_FILES = $(i386_FILES) tccpe.c
x86_64_FILES = $(CORE_FILES) x86_64-gen.c x86_64-link.c i386-asm.c x86_64-asm.h
x86_64-win32_FILES = $(x86_64_FILES) tccpe.c
x86_64-osx_FILES = $(x86_64_FILES)
arm_FILES = $(CORE_FILES) arm-gen.c arm-link.c arm-asm.c
arm-wince_FILES = $(arm_FILES) tccpe.c
arm64_FILES = $(CORE_FILES) arm64-gen.c arm64-link.c
c67_FILES = $(CORE_FILES) c67-gen.c c67-link.c tcccoff.c

# libtcc sources
LIBTCC_SRC = $(filter-out tcc.c tcctools.c arm-asm.c,$(filter %.c,$($T_FILES)))

ifeq ($(ONE_SOURCE),yes)
LIBTCC_OBJ = $(X)libtcc.o
LIBTCC_INC = $($T_FILES)
TCC_FILES = tcc.c
$(X)libtcc.o $T-tcc$(EXESUF) : DEFINES += -DONE_SOURCE
else
LIBTCC_OBJ = $(patsubst %.c,$(X)%.o,$(LIBTCC_SRC))
LIBTCC_INC = $(filter %.h %-gen.c %-link.c,$($T_FILES))
TCC_FILES = $(X)tcc.o $(LIBTCC_OBJ)
endif

# target specific object rule
$(X)%.o : %.c $(LIBTCC_INC)
	$(CC) -o $@ -c $< $(DEFINES) $(CFLAGS)

# additional dependencies
$(X)tcc.o : tcctools.c
$(X)arm-gen.o : arm-asm.c

# Host Tiny C Compiler
tcc$(EXESUF): tcc.o $(LIBTCC)
	$(CC) -o $@ $^ $(DEFINES) $(CFLAGS) $(LIBS) $(LDFLAGS) $(LINK_LIBTCC)

# Cross Tiny C Compilers
%-tcc$(EXESUF): FORCE
	@$(MAKE) --no-print-directory $@ CROSS_TARGET=$* ONE_SOURCE=$(or $(ONE_SOURCE),yes)

$(CROSS_TARGET)-tcc$(EXESUF): $(TCC_FILES)
	$(CC) -o $@ $^ $(DEFINES) $(CFLAGS) $(LIBS) $(LDFLAGS)

# profiling version
tcc_p$(EXESUF): $($T_FILES)
	$(CC) -o $@ $< $(DEFINES) -DONE_SOURCE $(CFLAGS_P) $(LIBS_P) $(LDFLAGS_P)

# static libtcc library
libtcc.a: $(LIBTCC_OBJ)
	$(AR) rcs $@ $^

# dynamic libtcc library
libtcc.so: $(LIBTCC_OBJ)
	$(CC) -shared -Wl,-soname,$@ -o $@ $^ $(CFLAGS) $(LDFLAGS)

libtcc.so: CFLAGS+=-fPIC

# windows dynamic libtcc library
libtcc.dll : $(LIBTCC_OBJ)
	$(CC) -shared -o $@ $^ $(CFLAGS) $(LDFLAGS)

libtcc.def : libtcc.dll tcc$(EXESUF)
	./tcc$(EXESUF) -impdef $< -o $@

libtcc.dll : DEFINES += -DLIBTCC_AS_DLL

# TinyCC runtime libraries
libtcc1.a : tcc$(EXESUF) FORCE
	@$(MAKE) -C lib

# Cross libtcc1.a
libtcc1-%.a : %-tcc$(EXESUF) FORCE
	@$(MAKE) -C lib CROSS_TARGET=$*

FORCE:

# --------------------------------------------------------------------------
# documentation and man page
tcc-doc.html: tcc-doc.texi
	-makeinfo --no-split --html --number-sections -o $@ $<

tcc.1: tcc-doc.texi
	-$(TOPSRC)/texi2pod.pl $< tcc.pod
	-pod2man --section=1 --center="Tiny C Compiler" --release="$(VERSION)" tcc.pod > $@

tcc-doc.info: tcc-doc.texi
	-makeinfo $<

# --------------------------------------------------------------------------
# install

INSTALL = install -m644
INSTALLBIN = install -m755 $(STRIP_$(STRIP_BINARIES))
STRIP_yes = -s
install-strip: install
install-strip: STRIP_BINARIES = yes

TRY-INSTALL = $(if $(wildcard $1),mkdir -p $2 && $(INSTALL) $1 $2)
LIBTCC1_W = $(wildcard $(filter %-win32.a %-wince.a,$(LIBTCC1_CROSS)))
LIBTCC1_U = $(wildcard $(filter-out $(LIBTCC1_W),$(LIBTCC1_CROSS)))
PROGS_X = $(wildcard $(PROGS_CROSS))

# install progs & libs
install-unx:
	mkdir -p "$(bindir)"
	$(INSTALLBIN) $(PROGS) $(PROGS_X) "$(bindir)"
	mkdir -p "$(tccdir)"
	$(INSTALL) $(LIBTCC1) $(LIBTCC1_U) "$(tccdir)"
	mkdir -p "$(tccdir)/include"
	$(INSTALL) $(TOPSRC)/include/*.h $(TOPSRC)/tcclib.h "$(tccdir)/include"
	mkdir -p "$(libdir)"
	$(INSTALL) $(LIBTCC) "$(libdir)"
	mkdir -p "$(includedir)"
	$(INSTALL) $(TOPSRC)/libtcc.h "$(includedir)"
	$(call TRY-INSTALL,tcc.1,"$(mandir)/man1")
	$(call TRY-INSTALL,tcc-doc.info,"$(infodir)")
	$(call TRY-INSTALL,tcc-doc.html,"$(docdir)")
ifneq "$(LIBTCC1_W)" ""
	mkdir -p "$(tccdir)/win32/lib"
	$(INSTALL) $(TOPSRC)/win32/lib/*.def $(LIBTCC1_W) "$(tccdir)/win32/lib"
	mkdir -p "$(tccdir)/win32/include"
	cp -r $(TOPSRC)/win32/include/. "$(tccdir)/win32/include"
	$(INSTALL) $(TOPSRC)/include/*.h $(TOPSRC)/tcclib.h "$(tccdir)/win32/include"
endif

# uninstall
uninstall-unx:
	@rm -fv $(foreach P,$(PROGS) $(PROGS_CROSS),"$(bindir)/$P")
	@rm -fv "$(libdir)/libtcc.a" "$(libdir)/libtcc.so" "$(includedir)/libtcc.h"
	@rm -fv "$(mandir)/man1/tcc.1" "$(infodir)/tcc-doc.info"
	@rm -fv "$(docdir)/tcc-doc.html"
	rm -r "$(tccdir)"

# install progs & libs on windows
install-win:
	mkdir -p "$(tccdir)"
	$(INSTALL) $(PROGS) $(subst libtcc.a,,$(LIBTCC)) $(PROGS_X) "$(tccdir)"
	mkdir -p "$(tccdir)/lib"
	$(INSTALL) $(TOPSRC)/win32/lib/*.def "$(tccdir)/lib"
	$(INSTALL) libtcc1.a $(LIBTCC1_W) "$(tccdir)/lib"
	mkdir -p "$(tccdir)/include"
	cp -r $(TOPSRC)/win32/include/. "$(tccdir)/include"
	$(INSTALL) $(TOPSRC)/include/*.h $(TOPSRC)/tcclib.h "$(tccdir)/include"
	mkdir -p "$(tccdir)/examples"
	cp -r $(TOPSRC)/win32/examples/. "$(tccdir)/examples"
	$(INSTALL) $(TOPSRC)/tests/libtcc_test.c "$(tccdir)/examples"
	mkdir -p "$(tccdir)/libtcc"
	$(INSTALL) $(TOPSRC)/libtcc.h $(subst .dll,.def,$(LIBTCC)) "$(tccdir)/libtcc"
	mkdir -p "$(tccdir)/doc"
	$(INSTALL) $(TOPSRC)/win32/tcc-win32.txt $(wildcard tcc-doc.html) "$(tccdir)/doc"
ifneq "$(LIBTCC1_U)" ""
	$(INSTALL) $(LIBTCC1_U) "$(tccdir)/lib"
	mkdir -p "$(tccdir)/lib/include";
	$(INSTALL) $(TOPSRC)/include/*.h $(TOPSRC)/tcclib.h "$(tccdir)/lib/include"
endif

# the msys-git shell works to configure && make except it does not have install
install-win : INSTALL = cp

# uninstall on windows
uninstall-win:
	rm -r "$(tccdir)/"*

# --------------------------------------------------------------------------
# other stuff

TAGFILES = *.[ch] include/*.h lib/*.[chS]
tags : ; ctags $(TAGFILES)
# cannot have both tags and TAGS on windows
ETAGS : ; etags $(TAGFILES)

# create release tarball from *current* git branch (including tcc-doc.html
# and converting two files to CRLF)
TCC-VERSION = tcc-$(VERSION)
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

config.mak:
	$(if $(wildcard $@),,@echo "Please run ./configure." && exit 1)

# in tests subdir
test:
	$(MAKE) -C tests

clean:
	rm -f $(PROGS) $(PROGS_CROSS) tcc_p$(EXESUF) tcc.pod
	rm -f  *~ *.o *.a *.so* *.out *.log lib*.def *.exe *.dll a.out tags TAGS
	@$(MAKE) -C tests $@
	@$(MAKE) -C lib $@

distclean: clean
	rm -f config.h config.mak config.texi tcc.1 tcc-doc.info tcc-doc.html

.PHONY: all clean test tar tags ETAGS distclean install uninstall FORCE

help:
	@echo "make"
	@echo "   build native compiler (from separate objects)"
	@echo ""
	@echo "make cross"
	@echo "   build cross compilers (from one source)"
	@echo ""
	@echo "make ONE_SOURCE=yes / no"
	@echo "   force building from one source / separate objects"
	@echo ""
	@echo "make cross-TARGET"
	@echo "   build one specific cross compiler for 'TARGET', as in"
	@echo "   $(TCC_X)"
	@echo ""
	@echo "Cross compiler configuration:"
	@echo "   make will read custom configuration for cross compilers from a file"
	@echo "   'config-cross.mak' if present.  For example for a windows->i386-linux"
	@echo "   cross-compiler that expects the linux files in <tccdir>/i386-linux:"
	@echo ""
	@echo "   ROOT-i386 = {B}/i386-linux"
	@echo "   CRT-i386  = {B}/i386-linux/usr/lib"
	@echo "   LIB-i386  = {B}/i386-linux/lib:{B}/i386-linux/usr/lib"
	@echo "   INC-i386  = {B}/lib/include:{B}/i386-linux/usr/include"
	@echo "   DEF-i386  += -D__linux__"
	@echo ""
	@echo "Other supported make targets:"
	@echo "   install install-strip test tags ETAGS tar clean distclean help"

# --------------------------------------------------------------------------
endif # ($(INCLUDED),no)
