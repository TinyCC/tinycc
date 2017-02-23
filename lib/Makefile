#
# Tiny C Compiler Makefile for libtcc1.a
#

TOP = ..
include $(TOP)/Makefile
VPATH = $(TOPSRC)/lib $(TOPSRC)/win32/lib

ifneq ($(CROSS),yes)
 TCC = $(TOP)/tcc$(EXESUF)
 OUT = ../libtcc1.a
 BCHECK_O = bcheck.o
else
 TCC = $(TOP)/$(TARGET)-tcc$(EXESUF)
 OUT = libtcc1-$(TARGET).a
endif

XCC = $(TCC) -B$(TOPSRC)
XAR = $(TCC) -ar

I386_O = libtcc1.o alloca86.o alloca86-bt.o $(BCHECK_O)
X86_64_O = libtcc1.o alloca86_64.o alloca86_64-bt.o $(BCHECK_O)
ARM_O = libtcc1.o armeabi.o alloca-arm.o
ARM64_O = lib-arm64.o
WIN_O = crt1.o crt1w.o wincrt1.o wincrt1w.o dllcrt1.o dllmain.o

ifeq "$(TARGET)" "i386-win32"
 OBJ = $(I386_O) chkstk.o $(WIN_O)
 TGT = -DTCC_TARGET_I386 -DTCC_TARGET_PE
 XCC = $(TCC) -B$(TOPSRC)/win32 -I$(TOPSRC)/include
else ifeq "$(TARGET)" "x86_64-win32"
 OBJ = $(X86_64_O) chkstk.o $(WIN_O)
 TGT = -DTCC_TARGET_X86_64 -DTCC_TARGET_PE
 XCC = $(TCC) -B$(TOPSRC)/win32 -I$(TOPSRC)/include
else ifeq "$(TARGET)" "arm-wince"
 OBJ = $(ARM_O) $(WIN_O)
 TGT = -DTCC_TARGET_ARM -DTCC_TARGET_PE
 XCC = $(TCC) -B$(TOPSRC)/win32 -I$(TOPSRC)/include
else ifeq "$(TARGET)" "i386"
 OBJ = $(I386_O)
 TGT = -DTCC_TARGET_I386
else ifeq "$(TARGET)" "x86_64"
 OBJ = $(X86_64_O)
 TGT = -DTCC_TARGET_X86_64
else ifeq "$(TARGET)" "arm-eabihf"
 OBJ = $(ARM_O)
 TGT = -DTCC_TARGET_ARM
 ifneq ($(CROSS),yes)
 # using gcc, need asm
 XCC = $(CC)
 XFLAGS = $(CFLAGS) -fPIC
 XAR = $(AR)
 endif
else ifeq "$(TARGET)" "arm64"
 OBJ = $(ARM64_O)
 TGT = -DTCC_TARGET_ARM64
else ifneq "$(TARGET)" ""
 $(error libtcc1.a not supported on target '$(TARGET)')
endif

ifeq ($(TARGETOS),Darwin)
 XFLAGS += -D_ANSI_SOURCE
 BCHECK_O =
endif

all : $(OUT)

$(OUT) : $(patsubst %.o,%-$(TARGET).o,$(OBJ))
	$(XAR) rcs $@ $^
%-$(TARGET).o : %.c
	$(XCC) -c $< -o $@ $(TGT) $(XFLAGS)
%-$(TARGET).o : %.S
	$(XCC) -c $< -o $@ $(TGT) $(XFLAGS)

crt1w-$(TARGET).o : crt1.c
wincrt1w-$(TARGET).o : wincrt1.c
bcheck-$(TARGET).o : XFLAGS += -w

clean :
	rm -f *.a *.o $(OUT)
