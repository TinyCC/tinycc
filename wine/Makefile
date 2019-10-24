#
# This Makefile builds native Windows tcc for both 32 and 64 bits generator.
# This makefile is adjusted from 'https://sourceforge.net/projects/wintcc/'
# It requires mingw64 (and static libs for dl and pthread) and wine installed on linux.
# by a second stage to compile tcc it self.
# Generated binaries do not require cygwin to run.
# You can launch 'tarball' target to build a tar.gz you can install on any
# Windows machines.

CPU	= $(shell if `x86_64-w64-mingw32-gcc -v 2>&1 | grep Target | grep -q x86_64`; then echo 64; else echo 32; fi)
VERSION	= $(shell cat ../VERSION)
BOOTCC	= x86_64-w64-mingw32-gcc
CFLAGS	= -s -static -fno-strict-aliasing -Wno-incompatible-pointer-types -DTCC_TARGET_PE
#WINPWD	= `cygpath --mixed $(PWD)`
#@echo "#define CONFIG_TCCDIR \"$(WINPWD)\""

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
	@mkdir -p libtcc
	@mkdir -p doc
	@mkdir -p include
	@mkdir -p lib
	@cp -r ../win32/include/* include
	@cp  ../include/*.h include
	@cp ../tcclib.h include
	@cp ../libtcc.h include
	@cp ../win32/lib/* lib
	@cp ../win32/tcc-win32.txt doc
	@cp ../tests/libtcc_test.c examples

PHONY += pre

bootstrap:
	@echo Bootstrapping 32bits and 64bits tools with $(BOOTCC) -m$(CPU)
	@$(BOOTCC) $(CFLAGS) -DTCC_TARGET_I386   -o i386-win32-tcc.exe   ../tcc.c
	@$(BOOTCC) $(CFLAGS) -DTCC_TARGET_X86_64 -o x86_64-win32-tcc.exe ../tcc.c
	@$(BOOTCC) $(CFLAGS) $(NATIVE)           -o tcc.exe              ../tcc.c
	@$(BOOTCC) $(CFLAGS) $(NATIVE) -DLIBTCC_AS_DLL -o libtcc.dll -shared ../libtcc.c

PHONY += bootstrap

TCC = wine ./tcc.exe

lib/libtcc1-32.a:
	@echo Building $*.a with tcc -m32
	@$(TCC) -O2 -B. -m32 -DTCC_TARGET_PE -DTCC_TARGET_I386 -c ../lib/libtcc1.c 
	@$(TCC) -O2 -B. -m32 -DTCC_TARGET_PE -DTCC_TARGET_I386 -c ../lib/alloca86.S 
	@$(TCC) -O2 -B. -m32 -DTCC_TARGET_PE -DTCC_TARGET_I386 -c ../lib/alloca86-bt.S 
	@$(TCC) -O2 -B. -m32 -DTCC_TARGET_PE -DTCC_TARGET_I386 -c lib/crt1.c 
	@$(TCC) -O2 -B. -m32 -DTCC_TARGET_PE -DTCC_TARGET_I386 -c lib/crt1w.c
	@$(TCC) -O2 -B. -m32 -DTCC_TARGET_PE -DTCC_TARGET_I386 -c lib/wincrt1.c 
	@$(TCC) -O2 -B. -m32 -DTCC_TARGET_PE -DTCC_TARGET_I386 -c lib/wincrt1w.c
	@$(TCC) -O2 -B. -m32 -DTCC_TARGET_PE -DTCC_TARGET_I386 -c lib/dllcrt1.c 
	@$(TCC) -O2 -B. -m32 -DTCC_TARGET_PE -DTCC_TARGET_I386 -c lib/dllmain.c 
	@$(TCC) -O2 -B. -m32 -DTCC_TARGET_PE -DTCC_TARGET_I386 -c lib/chkstk.S
	@$(TCC) -m32 -ar lib/libtcc1-32.a libtcc1.o alloca86.o alloca86-bt.o crt1.o wincrt1.o crt1w.o wincrt1w.o dllcrt1.o dllmain.o chkstk.o
	@rm *.o

lib/libtcc1-64.a:
	@echo Building $*.a with tcc -m64
	@$(TCC) -O2 -B. -m64 -DTCC_TARGET_PE -DTCC_TARGET_X86_64 -c ../lib/libtcc1.c 
	@$(TCC) -O2 -B. -m64 -DTCC_TARGET_PE -DTCC_TARGET_X86_64 -c ../lib/alloca86_64.S 
	@$(TCC) -O2 -B. -m64 -DTCC_TARGET_PE -DTCC_TARGET_X86_64 -c ../lib/alloca86_64-bt.S 
	@$(TCC) -O2 -B. -m64 -DTCC_TARGET_PE -DTCC_TARGET_X86_64 -c lib/crt1.c 
	@$(TCC) -O2 -B. -m64 -DTCC_TARGET_PE -DTCC_TARGET_X86_64 -c lib/crt1w.c
	@$(TCC) -O2 -B. -m64 -DTCC_TARGET_PE -DTCC_TARGET_X86_64 -c lib/wincrt1.c 
	@$(TCC) -O2 -B. -m64 -DTCC_TARGET_PE -DTCC_TARGET_X86_64 -c lib/wincrt1w.c
	@$(TCC) -O2 -B. -m64 -DTCC_TARGET_PE -DTCC_TARGET_X86_64 -c lib/dllcrt1.c 
	@$(TCC) -O2 -B. -m64 -DTCC_TARGET_PE -DTCC_TARGET_X86_64 -c lib/dllmain.c 
	@$(TCC) -O2 -B. -m64 -DTCC_TARGET_PE -DTCC_TARGET_X86_64 -c lib/chkstk.S 
	@$(TCC) -m64 -ar lib/libtcc1-64.a libtcc1.o alloca86_64.o alloca86_64-bt.o crt1.o wincrt1.o crt1w.o wincrt1w.o dllcrt1.o dllmain.o chkstk.o
	@rm *.o

libs: lib/libtcc1-32.a lib/libtcc1-64.a

PHONY += libs

rebuild:
	@echo Rebuild using tcc itself - default $(TARGET)bits
	@wine ./$(TARCH)-win32-tcc.exe -O2 -B. $(TFLAGS) -DTCC_TARGET_PE -o tcc.exe ../tcc.c
	@$(TCC) -O2 -B. $(TFLAGS) -DTCC_TARGET_PE -DLIBTCC_AS_DLL -o libtcc.dll -shared ../libtcc.c
	@$(TCC) -O2 -B. -m32 -DTCC_TARGET_PE -DTCC_TARGET_I386 -o tmp-tcc.exe ../tcc.c && mv tmp-tcc.exe i386-win32-tcc.exe
	@$(TCC) -O2 -B. -m$(TARGET) -DTCC_TARGET_PE -DTCC_TARGET_X86_64 -o tmp-tcc.exe ../tcc.c && mv tmp-tcc.exe x86_64-win32-tcc.exe

PHONY += rebuild

test:
	@(cd ../tests; $(MAKE) -k TCC=$(PWD)/tcc hello-exe hello-run)
	@(cd ../tests; $(MAKE) -k TCC=$(PWD)/tcc vla_test-run tests2-dir pp-dir)
ifeq ($(CPU), 64)
	@(cd ../tests; $(MAKE) -k TCC=$(PWD)/i386-win32-tcc hello-exe hello-run)
	@(cd ../tests; $(MAKE) -k TCC=$(PWD)/i386-win32-tcc vla_test-run tests2-dir pp-dir)
endif

PHONY += test

ARCHIVE	= tcc-$(VERSION)-$(ARCH)-win32

tarball:
	@rm -r -f $(ARCHIVE)
	@mkdir $(ARCHIVE)
	@cp -r doc $(ARCHIVE)
	@cp -r examples $(ARCHIVE)
	@cp -r include $(ARCHIVE)
	@cp -r lib $(ARCHIVE)
	@cp -r libtcc $(ARCHIVE)
	@cp -r i386-win32-tcc.exe $(ARCHIVE)
	@cp -r libtcc.def $(ARCHIVE)
	@cp -r libtcc.dll $(ARCHIVE)
	@cp -r Makefile $(ARCHIVE)
	@cp -r tcc.exe $(ARCHIVE)
	@cp -r doc/tcc-win32.txt $(ARCHIVE)
	@cp -r x86_64-win32-tcc.exe $(ARCHIVE)
	@tar czf $(ARCHIVE).tar.gz $(ARCHIVE)
	@zip -9 -q -r $(ARCHIVE).zip $(ARCHIVE)
	@rm -r -f $(ARCHIVE)
	@ls -g $(ARCHIVE)*

PHONY += tarball

clean:
	rm -f *.o *.exe *.dll lib/*.a *.pdb *.obj *.exp *.def *.lib
