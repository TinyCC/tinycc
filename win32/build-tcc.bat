@rem ----------------------------------------------------
@rem batch file to build tcc using gcc and ar from mingw
@rem ----------------------------------------------------
@set PROMPT=$G$S

echo>..\config.h #define TCC_VERSION "0.9.25"
echo>>..\config.h #define TCC_TARGET_PE 1
echo>>..\config.h #define CONFIG_TCCDIR "."
echo>>..\config.h #define CONFIG_SYSROOT ""

@if _%PROCESSOR_ARCHITEW6432%_==_AMD64_ goto x86_64
@if _%PROCESSOR_ARCHITECTURE%_==_AMD64_ goto x86_64
@goto tools

:x86_64
echo>>..\config.h #define TCC_TARGET_X86_64 1
@set P=x86_64-pc-mingw32-
@set S=_64
@goto tools

:tools
%P%gcc -Os tools/tiny_impdef.c -o tiny_impdef.exe -s
%P%gcc -Os tools/tiny_libmaker.c -o tiny_libmaker.exe -s

:libtcc
if not exist libtcc\nul mkdir libtcc
copy ..\libtcc.h libtcc\libtcc.h
%P%gcc -Os -fno-strict-aliasing ../libtcc.c -c -o libtcc.o
%P%ar rcs libtcc/libtcc.a libtcc.o

:tcc
%P%gcc -Os -fno-strict-aliasing ../tcc.c -o tcc.exe -s -DTCC_USE_LIBTCC -ltcc -Llibtcc

:copy_std_includes
copy ..\include\*.h include

:libtcc1.a
.\tcc -c lib/crt1.c
.\tcc -c lib/wincrt1.c
.\tcc -c lib/dllcrt1.c
.\tcc -c lib/dllmain.c
.\tcc -c ../lib/libtcc1.c
.\tcc -c lib/chkstk.S
.\tcc -c ../lib/alloca86%S%.S

@set LIBFILES=crt1.o wincrt1.o dllcrt1.o dllmain.o chkstk.o libtcc1.o alloca86%S%.o

@if not _%P%==_ goto makelib
.\tcc -c ../lib/alloca86-bt%S%.S
.\tcc -c ../lib/bcheck.c

@set LIBFILES=%LIBFILES% alloca86-bt%S%.o bcheck.o

:makelib
tiny_libmaker lib/libtcc1.a %LIBFILES%
del *.o
