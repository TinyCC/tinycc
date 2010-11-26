@rem ----------------------------------------------------
@rem batch file to build tcc using gcc and ar from mingw
@rem ----------------------------------------------------
@set PROMPT=$G$S

echo>..\config.h #define TCC_VERSION "0.9.25"
echo>>..\config.h #define CONFIG_TCCDIR "."
echo>>..\config.h #define CONFIG_SYSROOT ""
@set target=-DTCC_TARGET_PE -DTCC_TARGET_I386
@set CC=gcc
@set AR=ar

@if _%PROCESSOR_ARCHITEW6432%_==_AMD64_ goto x86_64
@if _%PROCESSOR_ARCHITECTURE%_==_AMD64_ goto x86_64
@goto tools

:x86_64
@set target=-DTCC_TARGET_PE -DTCC_TARGET_X86_64
@set CC=x86_64-pc-mingw32-gCC
@set AR=x86_64-pc-mingw32-ar
@set S=_64
@goto tools

:tools
%CC% %target% -Os tools/tiny_impdef.c -o tiny_impdef.exe -s
%CC% %target% -Os tools/tiny_libmaker.c -o tiny_libmaker.exe -s

:libtcc
if not exist libtcc\nul mkdir libtcc
copy ..\libtcc.h libtcc\libtcc.h
%CC% %target% -Os -fno-strict-aliasing ../libtcc.c -c -o libtcc.o
%AR% rcs libtcc/libtcc.a libtcc.o

:tcc
%CC% %target% -Os -fno-strict-aliasing ../tcc.c -o tcc.exe -s -DTCC_USE_LIBTCC -ltcc -Llibtcc

:copy_std_includes
copy ..\include\*.h include

:libtcc1.a
.\tcc %target% -c lib/crt1.c
.\tcc %target% -c lib/wincrt1.c
.\tcc %target% -c lib/dllcrt1.c
.\tcc %target% -c lib/dllmain.c
.\tcc %target% -c ../lib/libtcc1.c
.\tcc %target% -c lib/chkstk.S
.\tcc %target% -c ../lib/alloca86%S%.S

@set LIBFILES=crt1.o wincrt1.o dllcrt1.o dllmain.o chkstk.o libtcc1.o alloca86%S%.o

@if not _%P%==_ goto makelib
.\tcc %target% -c ../lib/alloca86-bt%S%.S
.\tcc %target% -c ../lib/bcheck.c

@set LIBFILES=%LIBFILES% alloca86-bt%S%.o bcheck.o

:makelib
tiny_libmaker lib/libtcc1.a %LIBFILES%
del *.o
