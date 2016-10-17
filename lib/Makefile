#
# Tiny C Compiler Makefile for libtcc1.a
#

TOP = ..
include $(TOP)/Makefile
VPATH = $(TOPSRC)/lib $(TOPSRC)/win32/lib

ifndef TARGET
 # we're building the native libtcc1.a
 ifdef CONFIG_WIN32
  ifeq ($(ARCH),x86-64)
   TARGET = x86_64-win32
  else
   TARGET = i386-win32
  endif
  # using tcc
 else ifeq ($(ARCH),i386)
  TARGET = i386
  XCC = $(CC) # using gcc
 else ifeq ($(ARCH),x86-64)
  TARGET = x86_64
  XCC = $(CC) # using gcc
 else ifeq ($(ARCH),arm)
  TARGET = arm
  XCC = $(CC) # using gcc
 else ifeq ($(ARCH),arm64)
  TARGET = arm64
 endif
 BCHECK_O = bcheck.o
endif

DIR = $(TARGET)

native : ../libtcc1.a
cross : $(DIR)/libtcc1.a

native : TCC = $(TOP)/tcc$(EXESUF)
cross : TCC = $(TOP)/$(TARGET)-tcc$(EXESUF)

I386_O = libtcc1.o alloca86.o alloca86-bt.o $(BCHECK_O)
X86_64_O = libtcc1.o alloca86_64.o alloca86_64-bt.o $(BCHECK_O)
ARM_O = libtcc1.o armeabi.o alloca-arm.o
ARM64_O = lib-arm64.o
WIN32_O = crt1.o wincrt1.o dllcrt1.o dllmain.o chkstk.o

# build TCC runtime library to contain PIC code, so it can be linked
# into shared libraries
PICFLAGS = -fPIC

ifeq "$(TARGET)" "i386-win32"
 OBJ = $(addprefix $(DIR)/,$(I386_O) $(WIN32_O))
 TGT = -DTCC_TARGET_I386 -DTCC_TARGET_PE
 XCC = $(TCC) -B$(TOPSRC)/win32 -I$(TOPSRC)/include
 XAR = $(DIR)/tiny_libmaker$(EXESUF)
 XFLAGS = $(TGT)
else ifeq "$(TARGET)" "x86_64-win32"
 OBJ = $(addprefix $(DIR)/,$(X86_64_O) $(WIN32_O))
 TGT = -DTCC_TARGET_X86_64 -DTCC_TARGET_PE
 XCC = $(TCC) -B$(TOPSRC)/win32 -I$(TOPSRC)/include
 XAR = $(DIR)/tiny_libmaker$(EXESUF)
 XFLAGS = $(TGT)
else ifeq "$(TARGET)" "i386"
 OBJ = $(addprefix $(DIR)/,$(I386_O))
 TGT = -DTCC_TARGET_I386
 XCC ?= $(TCC) -B$(TOPSRC)
else ifeq "$(TARGET)" "x86_64"
 OBJ = $(addprefix $(DIR)/,$(X86_64_O))
 TGT = -DTCC_TARGET_X86_64
 XCC ?= $(TCC) -B$(TOPSRC)
else ifeq "$(TARGET)" "arm"
 OBJ = $(addprefix $(DIR)/,$(ARM_O))
 TGT = -DTCC_TARGET_ARM
 XCC ?= $(TCC) -B$(TOPSRC)
else ifeq "$(TARGET)" "arm64"
 OBJ = $(addprefix $(DIR)/,$(ARM64_O))
 TGT = -DTCC_TARGET_ARM64
 XCC ?= $(TCC) -B$(TOPSRC)
else
 $(error libtcc1.a not supported on target '$(TARGET)')
endif

XFLAGS ?= $(CFLAGS) $(PICFLAGS) $(TGT)

ifeq ($(TARGETOS),Darwin)
 XAR = $(DIR)/tiny_libmaker$(EXESUF)
 XFLAGS += -D_ANSI_SOURCE
 BCHECK_O =
endif

ifdef XAR
AR = $(XAR)
endif

$(DIR)/libtcc1.a ../libtcc1.a : $(OBJ) $(XAR)
	$(AR) rcs $@ $(OBJ)
$(DIR)/%.o : %.c
	$(XCC) -c $< -o $@ $(XFLAGS)
$(DIR)/%.o : %.S
	$(XCC) -c $< -o $@ $(XFLAGS)
# windows : utilities
$(DIR)/tiny_%$(EXESUF) : $(TOPSRC)/win32/tools/tiny_%.c
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS) $(TGT)

$(OBJ) $(XAR) : $(DIR)/exists

%/exists :
	mkdir -p $(DIR)
	@echo $@ > $@

clean :
	rm -rf i386-win32 x86_64-win32 i386 x86_64 arm arm64
