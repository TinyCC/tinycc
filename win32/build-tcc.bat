@rem ------------------------------------------------------
@rem batch file to build tcc using mingw, msvc or tcc itself
@rem ------------------------------------------------------

@echo off
setlocal
if (%1)==(-clean) goto :cleanup
set CC=gcc -O2 -Wall
set /p VERSION= < ..\VERSION
set TCCDIR=
set BINDIR=
set DOC=no
set TX=
set SELF=%~nx0
goto :a0

:a2
shift
:a3
shift
:a0
if not (%1)==(-c) goto :a1
set CC=%~2
if (%2)==(cl) set CC=@call :cl
goto :a2
:a1
if (%1)==(-t) set T=%2&& goto :a2
if (%1)==(-v) set VERSION=%~2&& goto :a2
if (%1)==(-i) set TCCDIR=%2&& goto :a2
if (%1)==(-b) set BINDIR=%2&& goto :a2
if (%1)==(-d) set DOC=yes&& goto :a3
if (%1)==(-x) set TX=%2&& goto :a2
if (%1)==() goto :p1

:usage
echo usage: build-tcc.bat [ options ... ]
echo options:
echo   -c prog              use prog (gcc/tcc/cl) to compile tcc
echo   -c "prog options"    use prog with options to compile tcc
echo   -t target            set target
echo   -x target            build tcc cross-compiler for target
echo   -v "version"         set tcc version
echo   -i tccdir            install tcc into tccdir
echo   -b bindir            but install tcc.exe and libtcc.dll into bindir
echo   -d                   create tcc-doc.html too (needs makeinfo)
echo   -clean               delete all previously produced files and directories
echo   supported targets    i386 x86_64 arm64
exit /B 1

@rem ------------------------------------------------------
@rem sub-routines

:cleanup
set LOG=echo
%LOG% removing files:
for %%f in (*tcc.exe libtcc.dll lib\*.a) do call :del_file %%f
for %%f in (..\config.h ..\config.texi) do call :del_file %%f
for %%f in (include\*.h) do @if exist ..\%%f call :del_file %%f
for %%f in (include\tcclib.h examples\libtcc_test.c) do call :del_file %%f
for %%f in (lib\*.o *.o *.obj *.def *.pdb *.lib *.exp *.ilk) do call :del_file %%f
%LOG% removing directories:
for %%f in (doc libtcc) do call :del_dir %%f
%LOG% done.
exit /B 0
:del_file
if exist %1 del %1 && %LOG%   %1
exit /B 0
:del_dir
if exist %1 rmdir /Q/S %1 && %LOG%   %1
exit /B 0

@rem ------------------------------------------------------
:cl
@echo off
set CMD=cl
:c0
set ARG=%1
set ARG=%ARG:.dll=.lib%
if (%1)==(-shared) set ARG=-LD
if (%1)==(-o) shift && set ARG=-Fe%2
set CMD=%CMD% %ARG%
shift
if not (%1)==() goto :c0
echo on
%CMD% -O2 -W2 -Zi -MT -GS- -nologo %DEF_GITHASH% -link -opt:ref,icf
@exit /B %ERRORLEVEL%

@rem ------------------------------------------------------
@rem main program

:p1
if not _%TX%_==__ set T=%TX%&&set TX=%TX%-win32-
if _%T%_%PROCESSOR_ARCHITECTURE%_==__x86_ set T=i386
if _%T%_%PROCESSOR_ARCHITECTURE%_==__ARM64_ set T=arm64
if _%T%_==__ set T=x86_64
if %T%==i386 set D=-DTCC_TARGET_PE -DTCC_TARGET_I386
if %T%==x86_64 set D=-DTCC_TARGET_PE -DTCC_TARGET_X86_64
if %T%==arm64 set D=-DTCC_TARGET_PE -DTCC_TARGET_ARM64
if "%D%"=="" echo %SELF%: error: unknown target '%T%'&&exit /B 1

@if (%CC:~0,3%)==(gcc) set CC=%CC% -s -static
@if (%BINDIR%)==() set BINDIR=%TCCDIR%

:git_hash
git.exe --version 2>nul
if not %ERRORLEVEL%==0 goto :git_done
for /f %%b in ('git.exe rev-parse --abbrev-ref HEAD') do set GITHASH=%%b
for /f %%b in ('git.exe log -1 "--pretty=format:%%cs_%GITHASH%@%%h"') do set GITHASH=%%b
git.exe diff --quiet
if %ERRORLEVEL%==1 set GITHASH=%GITHASH%*
:git_done
@echo on

:config.h
echo>..\config.h #define TCC_VERSION "%VERSION%"
@if not "%GITHASH%"=="" echo>>..\config.h #define TCC_GITHASH "%GITHASH%"
@if not _%BINDIR%_==_%TCCDIR%_ echo>>..\config.h #define CONFIG_TCCDIR "%TCCDIR:\=/%"
@if not _%TX%_==__ @echo>>..\config.h #define CONFIG_TCC_CROSSPREFIX "%TX%"

@rem echo>> ..\config.h #define CONFIG_TCC_PREDEFS 1
@rem %CC% -DC2STR ..\conftest.c -o c2str.exe
@rem .\c2str.exe ../include/tccdefs.h ../tccdefs_.h

@if not _%TX%_==__ goto :tcc_cross
@if not _%TCC_C%_==__ goto :tcc_only

@if _%LIBTCC_C%_==__ set LIBTCC_C=..\libtcc.c
@set IMPLIB=libtcc.dll
@if "%CC:~0,5%"=="clang" set IMPLIB=libtcc.lib

%CC% -o libtcc.dll -shared %LIBTCC_C% %D% -DLIBTCC_AS_DLL
@if errorlevel 1 goto :the_end
%CC% -o tcc.exe ..\tcc.c %IMPLIB% %D% -DONE_SOURCE"=0"
@if errorlevel 1 goto :the_end
@goto :compiler_done

:tcc_only
%CC% -o tcc.exe %TCC_C% %D%
@if errorlevel 1 goto :the_end
@goto :compiler_done

:tcc_cross
%CC% -o %TX%tcc.exe ..\tcc.c %D%
@if errorlevel 1 goto :the_end
@goto :compiler_done

:compiler_done
@if (%EXES_ONLY%)==(yes) goto :files_done

if not exist libtcc mkdir libtcc
if not exist doc mkdir doc
copy>nul ..\include\*.h include
copy>nul ..\tcclib.h include
copy>nul ..\libtcc.h libtcc
copy>nul ..\tests\libtcc_test.c examples
copy>nul tcc-win32.txt doc

if exist libtcc.dll .\tcc -impdef libtcc.dll -o libtcc\libtcc.def
@if errorlevel 1 goto :the_end

:lib
@call :make_lib %TX% || goto :the_end

:tcc-doc.html
@if not (%DOC%)==(yes) goto :doc-done
echo>..\config.texi @set VERSION %VERSION%
cmd /c makeinfo --html --no-split ../tcc-doc.texi -o doc/tcc-doc.html
:doc-done

:files_done
for %%f in (*.o *.def) do @del %%f

:copy-install
@if (%TCCDIR%)==() goto :the_end
if not exist %BINDIR% mkdir %BINDIR%
for %%f in (*tcc.exe *tcc.dll) do @copy>nul %%f %BINDIR%\%%f
if not exist %TCCDIR% mkdir %TCCDIR%
@if not exist %TCCDIR%\lib mkdir %TCCDIR%\lib
for %%f in (lib\*.a lib\*.o lib\*.def) do @copy>nul %%f %TCCDIR%\%%f
for %%f in (include examples libtcc doc) do @xcopy>nul /s/i/q/y %%f %TCCDIR%\%%f

:the_end
exit /B %ERRORLEVEL%

:make_lib
@set LIBTCC1=libtcc1
@if _%1_==_arm64_ set LIBTCC1=lib-arm64
.\%1tcc -B. -c ../lib/%LIBTCC1%.c
.\%1tcc -B. -c lib/crt1.c
.\%1tcc -B. -c lib/crt1w.c
.\%1tcc -B. -c lib/wincrt1.c
.\%1tcc -B. -c lib/wincrt1w.c
.\%1tcc -B. -c lib/dllcrt1.c
.\%1tcc -B. -c lib/dllmain.c
.\%1tcc -B. -c lib/winex.c
.\%1tcc -B. -c lib/chkstk.S
.\%1tcc -B. -c ../lib/alloca.S
.\%1tcc -B. -c ../lib/alloca-bt.S
.\%1tcc -B. -c ../lib/stdatomic.c
.\%1tcc -B. -c ../lib/atomic.S
.\%1tcc -B. -c ../lib/builtin.c
.\%1tcc -ar lib/%1libtcc1.a %LIBTCC1%.o crt1.o crt1w.o wincrt1.o wincrt1w.o dllcrt1.o dllmain.o winex.o chkstk.o alloca.o alloca-bt.o stdatomic.o atomic.o builtin.o
.\%1tcc -B. -c ../lib/bcheck.c -o lib/%1bcheck.o -bt -I..
.\%1tcc -B. -c ../lib/bt-exe.c -o lib/%1bt-exe.o
.\%1tcc -B. -c ../lib/bt-log.c -o lib/%1bt-log.o
.\%1tcc -B. -c ../lib/bt-dll.c -o lib/%1bt-dll.o
.\%1tcc -B. -c ../lib/runmain.c -o lib/%1runmain.o
exit /B %ERRORLEVEL%
