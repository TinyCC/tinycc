#
# This Makefile builds native Windows tcc for both 32 and 64 bits generator.
# It requires Cygwin 64 and gcc to bootstrap a first tcc version which is used
# by a second stage to compile tcc it self.
# Generated binaries do not require cygwin to run.
# You can launch 'tarball' target to build a tar.gz you can install on any
# Windows machines.

ARCH	= $(shell if `gcc -v 2>&1 | grep Target | grep -q x86_64`; then echo 64; else echo 32; fi)
VERSION	= $(shell cat ../VERSION)
BOOTCC	= gcc
CFLAGS	= -s -fno-strict-aliasing -Wno-incompatible-pointer-types -DTCC_TARGET_PE -DONE_SOURCE

ifeq ($(ARCH), 64)
NATIVE	= -m$(ARCH) -DTCC_TARGET_X86_64
PREFIX	= x86_64
else
NATIVE	= -m$(ARCH) -DTCC_TARGET_I386
PREFIX	= i386
endif

all: pre bootstrap libs rebuild
	@chmod 775 *.exe
	@ls -ls *.exe

pre:
	@echo "#define TCC_VERSION \"$(VERSION)\"" 1>../config.h
	@echo "#ifdef TCC_TARGET_X86_64" 1>>../config.h
	@echo "#define CONFIG_TCC_LIBPATHS \"{B}/lib/64;{B}/lib\"" 1>>../config.h
	@echo "#else" 1>>../config.h
	@echo "#define CONFIG_TCC_LIBPATHS \"{B}/lib/32;{B}/lib\"" 1>>../config.h
	@echo "#endif" 1>>../config.h
	@echo @set VERSION $(VERSION) 1>../config.texi
	@rm -f *tcc.exe tiny_*.exe *tcc.dll
	@rm -r -f libtcc
	@rm -r -f libtcc doc lib/32 lib/64
	@mkdir libtcc
	@mkdir doc
	@mkdir lib/32
	@mkdir lib/64
	@cp ../include/*.h include
	@cp ../tcclib.h include
	@cp ../libtcc.h include
	@cp tcc-win32.txt doc
	@cp ../tests/libtcc_test.c examples

PHONY += pre

bootstrap:
	@echo Bootstrapping 32bits and 64bits tools with $(BOOTCC) -m$(ARCH)
	@$(BOOTCC) $(CFLAGS) -DTCC_TARGET_I386   -o i386-win32-tcc.exe   ../tcc.c
	@$(BOOTCC) $(CFLAGS) -DTCC_TARGET_I386   -o tiny_libmaker32.exe  ./tools/tiny_libmaker.c
	@$(BOOTCC) $(CFLAGS) -DTCC_TARGET_X86_64 -o x86_64-win32-tcc.exe ../tcc.c
	@$(BOOTCC) $(CFLAGS) -DTCC_TARGET_X86_64 -o tiny_libmaker64.exe  ./tools/tiny_libmaker.c
	@$(BOOTCC) $(CFLAGS) $(NATIVE)           -o tcc.exe              ../tcc.c
	@$(BOOTCC) $(CFLAGS) $(NATIVE) -DLIBTCC_AS_DLL -o libtcc.dll -shared ../libtcc.c

PHONY += bootstrap

lib/32/libtcc1.a: tiny_libmaker32.exe
	@echo Building $*.a with tcc -m32
	@./tcc -m32 -DTCC_TARGET_PE -DTCC_TARGET_I386 -c ../lib/libtcc1.c 
	@./tcc -m32 -DTCC_TARGET_PE -DTCC_TARGET_I386 -c ../lib/alloca86.S 
	@./tcc -m32 -DTCC_TARGET_PE -DTCC_TARGET_I386 -c ../lib/alloca86-bt.S 
	@./tcc -m32 -DTCC_TARGET_PE -DTCC_TARGET_I386 -c lib/crt1.c 
	@./tcc -m32 -DTCC_TARGET_PE -DTCC_TARGET_I386 -c lib/wincrt1.c 
	@./tcc -m32 -DTCC_TARGET_PE -DTCC_TARGET_I386 -c lib/dllcrt1.c 
	@./tcc -m32 -DTCC_TARGET_PE -DTCC_TARGET_I386 -c lib/dllmain.c 
	@./tcc -m32 -DTCC_TARGET_PE -DTCC_TARGET_I386 -c lib/chkstk.S 
	@./tiny_libmaker32 lib/32/libtcc1.a libtcc1.o alloca86.o alloca86-bt.o crt1.o wincrt1.o dllcrt1.o dllmain.o chkstk.o
	@rm *.o

lib/64/libtcc1.a: tcc.exe tiny_libmaker64.exe
	@echo Building $*.a with tcc -m64
	@./tcc -m64 -DTCC_TARGET_PE -DTCC_TARGET_X86_64 -c ../lib/libtcc1.c 
	@./tcc -m64 -DTCC_TARGET_PE -DTCC_TARGET_X86_64 -c ../lib/alloca86_64.S 
	@./tcc -m64 -DTCC_TARGET_PE -DTCC_TARGET_X86_64 -c ../lib/alloca86_64-bt.S 
	@./tcc -m64 -DTCC_TARGET_PE -DTCC_TARGET_X86_64 -c lib/crt1.c 
	@./tcc -m64 -DTCC_TARGET_PE -DTCC_TARGET_X86_64 -c lib/wincrt1.c 
	@./tcc -m64 -DTCC_TARGET_PE -DTCC_TARGET_X86_64 -c lib/dllcrt1.c 
	@./tcc -m64 -DTCC_TARGET_PE -DTCC_TARGET_X86_64 -c lib/dllmain.c 
	@./tcc -m64 -DTCC_TARGET_PE -DTCC_TARGET_X86_64 -c lib/chkstk.S 
	@./tiny_libmaker64 lib/64/libtcc1.a libtcc1.o alloca86_64.o alloca86_64-bt.o crt1.o wincrt1.o dllcrt1.o dllmain.o chkstk.o
	@rm *.o

libs: lib/32/libtcc1.a lib/64/libtcc1.a

PHONY += libs

rebuild:
	@echo Rebuild using tcc itself - default $(ARCH)bits
	@./$(PREFIX)-win32-tcc $(NATIVE) -DTCC_TARGET_PE -DONE_SOURCE -o tcc.exe ../tcc.c libtcc.dll
	@./$(PREFIX)-win32-tcc $(NATIVE) -DTCC_TARGET_PE -DONE_SOURCE -o tiny_libmaker.exe tools/tiny_libmaker.c
	@./tcc $(NATIVE) -DTCC_TARGET_PE -DONE_SOURCE -DLIBTCC_AS_DLL -o libtcc.dll -shared ../libtcc.c
	@./tcc -m$(ARCH) -DTCC_TARGET_PE -DONE_SOURCE -DTCC_TARGET_I386   -o tmp-tcc.exe ../tcc.c && mv tmp-tcc.exe i386-win32-tcc.exe
	@./tcc -m$(ARCH) -DTCC_TARGET_PE -DONE_SOURCE -DTCC_TARGET_X86_64 -o tmp-tcc.exe ../tcc.c && mv tmp-tcc.exe x86_64-win32-tcc.exe
	@./tcc -m$(ARCH) -DTCC_TARGET_PE -DONE_SOURCE -DTCC_TARGET_I386   -o tiny_libmaker32.exe tools/tiny_libmaker.c
	@./tcc -m$(ARCH) -DTCC_TARGET_PE -DONE_SOURCE -DTCC_TARGET_X86_64 -o tiny_libmaker64.exe tools/tiny_libmaker.c

PHONY += rebuild

TARNAME	= tcc-$(VERSION)-$(PREFIX)-win32

tarball:
	@rm -r -f $(TARNAME)
	@mkdir $(TARNAME)
	@cp -r doc $(TARNAME)
	@cp -r examples $(TARNAME)
	@cp -r include $(TARNAME)
	@cp -r lib $(TARNAME)
	@cp -r libtcc $(TARNAME)
	@cp -r tools $(TARNAME)
	@cp -r vs2015 $(TARNAME)
	@cp -r build-tcc.bat $(TARNAME)
	@cp -r i386-win32-tcc.exe $(TARNAME)
	@cp -r libtcc.def $(TARNAME)
	@cp -r libtcc.dll $(TARNAME)
	@cp -r Makefile $(TARNAME)
	@cp -r tcc.exe $(TARNAME)
	@cp -r tcc-win32.txt $(TARNAME)
	@cp -r tiny_libmaker.exe $(TARNAME)
	@cp -r tiny_libmaker32.exe $(TARNAME)
	@cp -r tiny_libmaker64.exe $(TARNAME)
	@cp -r x86_64-win32-tcc.exe $(TARNAME)
	@tar czf $(TARNAME).tar.gz $(TARNAME)
	@rm -r -f $(TARNAME)

PHONY += tarball

clean:
	rm -f *.o *.exe *.dll lib/*/*.a
