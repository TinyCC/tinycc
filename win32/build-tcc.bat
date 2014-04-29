@rem ----------------------------------------------------
@rem batch file to build tcc using mingw gcc
@rem ----------------------------------------------------

@set /p VERSION= < ..\VERSION
echo>..\config.h #define TCC_VERSION "%VERSION%"

@rem @if _%PROCESSOR_ARCHITEW6432%_==_AMD64_ goto x86_64
@if _%PROCESSOR_ARCHITECTURE%_==_AMD64_ goto x86_64

@set target=-DTCC_TARGET_PE -DTCC_TARGET_I386
@set CC=gcc -Os -s -fno-strict-aliasing
@set P=32
@goto tools

:x86_64
@set target=-DTCC_TARGET_PE -DTCC_TARGET_X86_64
@rem mingw 64 has an ICE with -Os
@set CC=x86_64-pc-mingw32-gcc -O0 -s -fno-strict-aliasing
@set P=64
@goto tools

:tools
%CC% %target% tools/tiny_impdef.c -o tiny_impdef.exe
%CC% %target% tools/tiny_libmaker.c -o tiny_libmaker.exe

:libtcc
if not exist libtcc mkdir libtcc
copy ..\libtcc.h libtcc\libtcc.h
%CC% %target% -shared -DLIBTCC_AS_DLL -DONE_SOURCE ../libtcc.c -o libtcc.dll -Wl,-out-implib,libtcc/libtcc.a
tiny_impdef libtcc.dll -o libtcc/libtcc.def

:tcc
%CC% %target% ../tcc.c -o tcc.exe -ltcc -Llibtcc

:copy_std_includes
copy ..\include\*.h include

:libtcc1.a
.\tcc %target% -c ../lib/libtcc1.c
.\tcc %target% -c lib/crt1.c
.\tcc %target% -c lib/wincrt1.c
.\tcc %target% -c lib/dllcrt1.c
.\tcc %target% -c lib/dllmain.c
.\tcc %target% -c lib/chkstk.S
goto lib%P%

:lib32
.\tcc %target% -c ../lib/alloca86.S
.\tcc %target% -c ../lib/alloca86-bt.S
.\tcc %target% -c ../lib/bcheck.c
tiny_libmaker lib/libtcc1.a libtcc1.o alloca86.o alloca86-bt.o crt1.o wincrt1.o dllcrt1.o dllmain.o chkstk.o bcheck.o
@goto the_end

:lib64
.\tcc %target% -c ../lib/alloca86_64.S
tiny_libmaker lib/libtcc1.a libtcc1.o alloca86_64.o crt1.o wincrt1.o dllcrt1.o dllmain.o chkstk.o

:the_end
del *.o

:makedoc
echo>..\config.texi @set VERSION %VERSION%
if not exist doc md doc
makeinfo --html --no-split -o doc\tcc-doc.html ../tcc-doc.texi
copy tcc-win32.txt doc
copy ..\tests\libtcc_test.c examples
