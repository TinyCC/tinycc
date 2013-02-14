#
# Tiny C Compiler Makefile for libtcc1.a
#

TOP = ..
include $(TOP)/Makefile
VPATH = $(top_srcdir)/lib $(top_srcdir)/win32/lib

ifndef TARGET
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
 endif
 endif
 endif
 endif
 BCHECK_O = bcheck.o
endif

DIR = $(TARGET)

native : ../libtcc1.a
cross : $(DIR)/libtcc1.a

native : TCC = $(TOP)/tcc$(EXESUF)
cross : TCC = $(TOP)/$(TARGET)-tcc$(EXESUF)

I386_O = libtcc1.o alloca86.o alloca86-bt.o $(BCHECK_O)
X86_64_O = libtcc1.o alloca86_64.o
WIN32_O = $(I386_O) crt1.o wincrt1.o dllcrt1.o dllmain.o chkstk.o
WIN64_O = $(X86_64_O) crt1.o wincrt1.o dllcrt1.o dllmain.o chkstk.o

ifeq "$(TARGET)" "i386-win32"
 OBJ = $(addprefix $(DIR)/,$(WIN32_O))
 TGT = -DTCC_TARGET_I386 -DTCC_TARGET_PE
 XCC = $(TCC) -B$(top_srcdir)/win32 -I$(top_srcdir)/include
 XAR = $(DIR)/tiny_libmaker$(EXESUF)
else
ifeq "$(TARGET)" "x86_64-win32"
 OBJ = $(addprefix $(DIR)/,$(WIN64_O))
 TGT = -DTCC_TARGET_X86_64 -DTCC_TARGET_PE
 XCC = $(TCC) -B$(top_srcdir)/win32 -I$(top_srcdir)/include
 XAR = $(DIR)/tiny_libmaker$(EXESUF)
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
 $(error libtcc1.a not supported on target '$(TARGET)')
endif
endif
endif
endif

XFLAGS = $(CPPFLAGS) $(CFLAGS) $(TGT)

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
$(DIR)/%$(EXESUF) : $(TOP)/win32/tools/%.c
	$(CC) -o $@ $< $(XFLAGS) $(LDFLAGS)

$(OBJ) $(XAR) : $(DIR)/exists
$(DIR)/exists :
	mkdir -p $(DIR)
	@echo $@ > $@

clean :
	rm -rfv i386-win32 x86_64-win32 i386 x86_64
