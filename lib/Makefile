#
# Tiny C Compiler Makefile for libtcc1.a
#

TOP = ..
include $(TOP)/Makefile
VPATH = $(TOPSRC)/lib $(TOPSRC)/win32/lib
T = $(or $(CROSS_TARGET),$(NATIVE_TARGET),unknown)
X = $(if $(CROSS_TARGET),$(CROSS_TARGET)-)

TCC = $(TOP)/$(X)tcc$(EXESUF)
XCC = $(TCC)
XAR = $(TCC) -ar
XFLAGS-unx = -B$(TOPSRC)
XFLAGS-win = -B$(TOPSRC)/win32 -I$(TOPSRC)/include
XFLAGS = $(XFLAGS$(XCFG))
XCFG = $(or $(findstring -win,$T),-unx)

ifeq ($(X),)
 BCHECK_O = bcheck.o
 ifeq "$T" "arm"
  XCC = $(CC)
  XAR = $(AR)
  XFLAGS = $(CFLAGS) -fPIC
 endif
endif

ifeq ($(TARGETOS),Darwin)
 XFLAGS += -D_ANSI_SOURCE
 BCHECK_O =
endif

ifeq ($(CONFIG_musl),yes)
 BCHECK_O =
endif

I386_O = libtcc1.o alloca86.o alloca86-bt.o $(BCHECK_O)
X86_64_O = libtcc1.o alloca86_64.o alloca86_64-bt.o $(BCHECK_O)
ARM_O = libtcc1.o armeabi.o alloca-arm.o
ARM64_O = lib-arm64.o
WIN_O = crt1.o crt1w.o wincrt1.o wincrt1w.o dllcrt1.o dllmain.o

OBJ-i386 = $(I386_O)
TGT-i386 = -DTCC_TARGET_I386

OBJ-x86_64 = $(X86_64_O)
TGT-x86_64 = -DTCC_TARGET_X86_64

OBJ-x86_64-osx = $(X86_64_O)
TGT-x86_64-osx = -DTCC_TARGET_X86_64 -DTCC_TARGET_MACHO

OBJ-arm = $(ARM_O)
TGT-arm = -DTCC_TARGET_ARM

OBJ-arm64 = $(ARM64_O)
TGT-arm64 = -DTCC_TARGET_ARM64

OBJ-i386-win32 = $(I386_O) chkstk.o $(WIN_O)
TGT-i386-win32 = -DTCC_TARGET_I386 -DTCC_TARGET_PE

OBJ-x86_64-win32 = $(X86_64_O) chkstk.o $(WIN_O)
TGT-x86_64-win32 = -DTCC_TARGET_X86_64 -DTCC_TARGET_PE

OBJ-arm-wince = $(ARM_O) $(WIN_O)
TGT-arm-wince = -DTCC_TARGET_ARM -DTCC_TARGET_PE

all : $(BIN)

$(BIN) : $(patsubst %.o,$(X)%.o,$(OBJ-$T))
	$(XAR) rcs $@ $^

$(X)%.o : %.c
	$(XCC) -c $< -o $@ $(TGT-$T) $(XFLAGS)

$(X)%.o : %.S
	$(XCC) -c $< -o $@ $(TGT-$T) $(XFLAGS)

$(X)crt1w.o : crt1.c
$(X)wincrt1w.o : wincrt1.c

clean :
	rm -f *.a *.o $(BIN)
