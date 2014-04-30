#
# Tiny C Compiler Makefile for libcrt.a
#

TOP = ..
include $(TOP)/Makefile
VPATH = $(top_srcdir)/lib $(top_srcdir)/win32/lib

ifndef TARGET # native library
 ifdef CONFIG_WIN64
  TARGET = x86_64-win32
 else
 ifdef CONFIG_WIN32
  TARGET = i386-win32
 else
 ifeq ($(ARCH),i386)
  TARGET = i386
  ifneq ($(TARGETOS),Darwin)
   XCC = $(CC)
  endif
 else
 ifeq ($(ARCH),x86-64)
  TARGET = x86_64
  ifneq ($(TARGETOS),Darwin)
   XCC = $(CC)
  endif
 else
 ifeq ($(ARCH),arm)
  TARGET = arm
  XCC = $(CC)
 endif
 endif
 endif
 endif
 endif
 BCHECK_O = bcheck.o
endif

DIR = $(TARGET)

native : ../libcrt.a
cross : $(DIR)/libcrt.a

native : TCC = $(TOP)/tcc$(EXESUF)
cross : TCC = $(TOP)/$(TARGET)-tcc$(EXESUF)

I386_O = libcrt.o alloca86.o alloca86-bt.o $(BCHECK_O)
X86_64_O = libcrt.o alloca86_64.o
ARM_O = libcrt.o armeabi.o alloca-arm.o
WIN32_O = $(I386_O) crt1.o wincrt1.o dllcrt1.o dllmain.o chkstk.o
WIN64_O = $(X86_64_O) crt1.o wincrt1.o dllcrt1.o dllmain.o chkstk.o

# build TCC runtime library to contain PIC code, so it can be linked
# into shared libraries
PICFLAGS = -fPIC

ifeq "$(TARGET)" "i386-win32"
 OBJ = $(addprefix $(DIR)/,$(WIN32_O))
 TGT = -DTCC_TARGET_I386 -DTCC_TARGET_PE
 XCC = $(TCC) -B$(top_srcdir)/win32 -I$(top_srcdir)/include
 XAR = $(DIR)/tiny_libmaker$(EXESUF)
 PICFLAGS =
else
ifeq "$(TARGET)" "x86_64-win32"
 OBJ = $(addprefix $(DIR)/,$(WIN64_O))
 TGT = -DTCC_TARGET_X86_64 -DTCC_TARGET_PE
 XCC = $(TCC) -B$(top_srcdir)/win32 -I$(top_srcdir)/include
 XAR = $(DIR)/tiny_libmaker$(EXESUF)
 PICFLAGS =
else
ifeq "$(TARGET)" "i386"
 OBJ = $(addprefix $(DIR)/,$(I386_O))
 TGT = -DTCC_TARGET_I386
 XCC ?= $(TCC) -B$(TOP)
else
ifeq "$(TARGET)" "x86_64"
 OBJ = $(addprefix $(DIR)/,$(X86_64_O))
 TGT = -DTCC_TARGET_X86_64
 XCC ?= $(TCC) -B$(TOP)
else
ifeq "$(TARGET)" "arm"
 OBJ = $(addprefix $(DIR)/,$(ARM_O))
 TGT = -DTCC_TARGET_ARM
 XCC ?= $(TCC) -B$(TOP)
else
 $(error libcrt.a not supported on target '$(TARGET)')
endif
endif
endif
endif
endif

XFLAGS = $(CPPFLAGS) $(CFLAGS) $(PICFLAGS) $(TGT)

ifeq ($(TARGETOS),Darwin)
 XAR = $(DIR)/tiny_libmaker$(EXESUF)
 XFLAGS += -D_ANSI_SOURCE
 BCHECK_O =
endif

ifdef XAR
AR = $(XAR)
endif

$(DIR)/libcrt.a ../libcrt.a : $(OBJ) $(XAR)
	$(AR) rcs $@ $(OBJ)
$(DIR)/%.o : %.c
	$(XCC) -c $< -o $@ $(XFLAGS)
$(DIR)/%.o : %.S
	$(XCC) -c $< -o $@ $(XFLAGS)
$(DIR)/%$(EXESUF) : $(TOP)/win32/tools/%.c
	$(CC) -o $@ $< $(XFLAGS) $(LDFLAGS)

$(OBJ) $(XAR) : $(DIR)/exists
$(DIR)/exists :
	mkdir -p $(DIR)
	@echo $@ > $@

clean :
	rm -rfv i386-win32 x86_64-win32 i386 x86_64
