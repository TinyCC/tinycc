#
# This Makefile builds native Windows tcc for both 32 and 64 bits generator.
# It requires Cygwin 64 and gcc to bootstrap a first tcc version which is used
# by a second stage to compile tcc it self.
# Generated binaries do not require cygwin to run.
# You can launch 'tarball' target to build a tar.gz you can install on any
# Windows machines.

CPU	= $(shell if `gcc -v 2>&1 | grep Target | grep -q x86_64`; then echo 64; else echo 32; fi)
VERSION	= $(shell cat ../VERSION)
BOOTCC	= gcc
CFLAGS	= -s -static -fno-strict-aliasing -Wno-incompatible-pointer-types -DTCC_TARGET_PE -DONE_SOURCE

ifeq ($(CPU), 64)
NATIVE	= -m$(CPU) -DTCC_TARGET_X86_64
ARCH	= x86_64
else
NATIVE	= -m$(CPU) -DTCC_TARGET_I386
ARCH	= i386
endif

TARGET	= $(CPU)

ifeq ($(TARGET), 64)
TFLAGS	= -m$(TARGET) -DTCC_TARGET_X86_64
TARCH	= x86_64
else
TFLAGS	= -m$(TARGET) -DTCC_TARGET_I386
TARCH	= i386
endif

all: pre bootstrap libs rebuild
	@chmod 775 *.exe
	@ls -ls *.exe

pre:
	@echo ARCH=$(TARCH) 1> ../config.mak
	@echo TARGETOS=Windows  1>> ../config.mak
	@echo CONFIG_WIN32=yes 1>> ../config.mak
	@echo TOPSRC=$$\(TOP\) 1>> ../config.mak
	@echo "#define TCC_VERSION \"$(VERSION)\"" 1>../config.h
	@echo "#ifdef TCC_TARGET_X86_64" 1>>../config.h
	@echo "#define TCC_LIBTCC1 \"libtcc1-64.a\"" 1>>../config.h
	@echo "#else" 1>>../config.h
	@echo "#define TCC_LIBTCC1 \"libtcc1-32.a\"" 1>>../config.h
	@echo "#endif" 1>>../config.h
	@echo @set VERSION $(VERSION) 1>../config.texi
	@rm -f *tcc.exe tiny_*.exe *tcc.dll lib/*.a
	@rm -r -f libtcc doc
	@mkdir libtcc
	@mkdir doc
	@cp ../include/*.h include
	@cp ../tcclib.h include
	@cp ../libtcc.h include
	@cp tcc-win32.txt doc
	@cp ../tests/libtcc_test.c examples

PHONY += pre

bootstrap:
	@echo Bootstrapping 32bits and 64bits tools with $(BOOTCC) -m$(CPU)
	@$(BOOTCC) $(CFLAGS) -DTCC_TARGET_I386   -o i386-win32-tcc.exe   ../tcc.c
	@$(BOOTCC) $(CFLAGS) -DTCC_TARGET_X86_64 -o x86_64-win32-tcc.exe ../tcc.c
	@$(BOOTCC) $(CFLAGS) $(NATIVE)           -o tcc.exe              ../tcc.c
	@$(BOOTCC) $(CFLAGS) $(NATIVE) -DLIBTCC_AS_DLL -o libtcc.dll -shared ../libtcc.c

PHONY += bootstrap

lib/libtcc1-32.a:
	@echo Building $*.a with tcc -m32
	@./tcc -m32 -DTCC_TARGET_PE -DTCC_TARGET_I386 -c ../lib/libtcc1.c 
	@./tcc -m32 -DTCC_TARGET_PE -DTCC_TARGET_I386 -c ../lib/alloca86.S 
	@./tcc -m32 -DTCC_TARGET_PE -DTCC_TARGET_I386 -c ../lib/alloca86-bt.S 
	@./tcc -m32 -DTCC_TARGET_PE -DTCC_TARGET_I386 -c lib/crt1.c 
	@./tcc -m32 -DTCC_TARGET_PE -DTCC_TARGET_I386 -c lib/crt1w.c
	@./tcc -m32 -DTCC_TARGET_PE -DTCC_TARGET_I386 -c lib/wincrt1.c 
	@./tcc -m32 -DTCC_TARGET_PE -DTCC_TARGET_I386 -c lib/wincrt1w.c
	@./tcc -m32 -DTCC_TARGET_PE -DTCC_TARGET_I386 -c lib/dllcrt1.c 
	@./tcc -m32 -DTCC_TARGET_PE -DTCC_TARGET_I386 -c lib/dllmain.c 
	@./tcc -m32 -DTCC_TARGET_PE -DTCC_TARGET_I386 -c lib/chkstk.S
	@./tcc -m32 -ar lib/libtcc1-32.a libtcc1.o alloca86.o alloca86-bt.o crt1.o wincrt1.o crt1w.o wincrt1w.o dllcrt1.o dllmain.o chkstk.o
	@rm *.o

lib/libtcc1-64.a:
	@echo Building $*.a with tcc -m64
	@./tcc -m64 -DTCC_TARGET_PE -DTCC_TARGET_X86_64 -c ../lib/libtcc1.c 
	@./tcc -m64 -DTCC_TARGET_PE -DTCC_TARGET_X86_64 -c ../lib/alloca86_64.S 
	@./tcc -m64 -DTCC_TARGET_PE -DTCC_TARGET_X86_64 -c ../lib/alloca86_64-bt.S 
	@./tcc -m64 -DTCC_TARGET_PE -DTCC_TARGET_X86_64 -c lib/crt1.c 
	@./tcc -m64 -DTCC_TARGET_PE -DTCC_TARGET_X86_64 -c lib/crt1w.c
	@./tcc -m64 -DTCC_TARGET_PE -DTCC_TARGET_X86_64 -c lib/wincrt1.c 
	@./tcc -m64 -DTCC_TARGET_PE -DTCC_TARGET_X86_64 -c lib/wincrt1w.c
	@./tcc -m64 -DTCC_TARGET_PE -DTCC_TARGET_X86_64 -c lib/dllcrt1.c 
	@./tcc -m64 -DTCC_TARGET_PE -DTCC_TARGET_X86_64 -c lib/dllmain.c 
	@./tcc -m64 -DTCC_TARGET_PE -DTCC_TARGET_X86_64 -c lib/chkstk.S 
	@./tcc -m64 -ar lib/libtcc1-64.a libtcc1.o alloca86_64.o alloca86_64-bt.o crt1.o wincrt1.o crt1w.o wincrt1w.o dllcrt1.o dllmain.o chkstk.o
	@rm *.o

libs: lib/libtcc1-32.a lib/libtcc1-64.a

PHONY += libs

rebuild:
	@echo Rebuild using tcc itself - default $(TARGET)bits
	@./$(TARCH)-win32-tcc $(TFLAGS) -DTCC_TARGET_PE -DONE_SOURCE -o tcc.exe ../tcc.c
	@./tcc $(TFLAGS) -DTCC_TARGET_PE -DONE_SOURCE -DLIBTCC_AS_DLL -o libtcc.dll -shared ../libtcc.c
	@./tcc -m$(TARGET) -DTCC_TARGET_PE -DONE_SOURCE -DTCC_TARGET_I386   -o tmp-tcc.exe ../tcc.c && mv tmp-tcc.exe i386-win32-tcc.exe
	@./tcc -m$(TARGET) -DTCC_TARGET_PE -DONE_SOURCE -DTCC_TARGET_X86_64 -o tmp-tcc.exe ../tcc.c && mv tmp-tcc.exe x86_64-win32-tcc.exe

PHONY += rebuild

test:
	@(cd ../tests/tests2; $(MAKE) -k TCC=../../win32/tcc)

PHONY += test

TARNAME	= tcc-$(VERSION)-$(ARCH)-win32

tarball:
	@rm -r -f $(TARNAME)
	@mkdir $(TARNAME)
	@cp -r doc $(TARNAME)
	@cp -r examples $(TARNAME)
	@cp -r include $(TARNAME)
	@cp -r lib $(TARNAME)
	@cp -r libtcc $(TARNAME)
	@cp -r tools $(TARNAME)
	@cp -r build-tcc.bat $(TARNAME)
	@cp -r i386-win32-tcc.exe $(TARNAME)
	@cp -r libtcc.def $(TARNAME)
	@cp -r libtcc.dll $(TARNAME)
	@cp -r Makefile $(TARNAME)
	@cp -r tcc.exe $(TARNAME)
	@cp -r tcc-win32.txt $(TARNAME)
	@cp -r x86_64-win32-tcc.exe $(TARNAME)
	@tar czf $(TARNAME).tar.gz $(TARNAME)
	@rm -r -f $(TARNAME)

PHONY += tarball

clean:
	rm -f *.o *.exe *.dll lib/*.a *.pdb *.obj *.exp *.def *.lib
