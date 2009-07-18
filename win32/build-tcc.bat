@rem ----------------------------------------------------
@rem batch file to build tcc using gcc and ar from mingw
@rem ----------------------------------------------------
@set PROMPT=$G$S

copy ..\include\*.h include

echo>..\config.h #define TCC_VERSION "0.9.25"
echo>>..\config.h #define TCC_TARGET_PE 1
echo>>..\config.h #define CONFIG_TCCDIR "."
echo>>..\config.h #define CONFIG_SYSROOT ""

@if _%PROCESSOR_ARCHITEW6432%_==_AMD64_ goto x86_64
@if _%PROCESSOR_ARCHITECTURE%_==_AMD64_ goto x86_64

:i386
@set AS=.\tcc -c
@goto tools

:x86_64
@set P=x86_64-pc-mingw32-
@set X64=-x64
@rem !!! currently TinyCC doesn't 64bit asm. Need 'yasm' !!!
@set AS=yasm -p gnu -f elf64
echo>>..\config.h #define TCC_TARGET_X86_64 1
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

:libtcc1.a
.\tcc -c lib/crt1.c
.\tcc -c lib/wincrt1.c
.\tcc -c lib/dllcrt1.c
.\tcc -c lib/dllmain.c
.\tcc -c ../lib/libtcc1.c
.\tcc -o tmp.s -E lib/chkstk.S
%AS% -o chkstk.o tmp.s
.\tcc -o tmp.s -E ../lib/alloca86.S
%AS% -o alloca86.o tmp.s
del tmp.s
tiny_libmaker lib/libtcc1%X64%.a crt1.o wincrt1.o dllcrt1.o dllmain.o chkstk.o libtcc1.o alloca86.o

:cleanup
del *.o
