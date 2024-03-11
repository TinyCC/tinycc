@rem ------------------------------------------------------
@rem batch file to build tcc using mingw, msvc or tcc itself
@rem ------------------------------------------------------

@echo off
setlocal
if (%1)==(-clean) goto :cleanup
set CC=gcc
set /p VERSION= < ..\VERSION
set TCCDIR=
set BINDIR=
set DOC=no
set XCC=no
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
if (%1)==(-x) set XCC=yes&& goto :a3
if (%1)==() goto :p1
:usage
echo usage: build-tcc.bat [ options ... ]
echo options:
echo   -c prog              use prog (gcc/tcc/cl) to compile tcc
echo   -c "prog options"    use prog with options to compile tcc
echo   -t 32/64             force 32/64 bit default target
echo   -v "version"         set tcc version
echo   -i tccdir            install tcc into tccdir
echo   -b bindir            but install tcc.exe and libtcc.dll into bindir
echo   -d                   create tcc-doc.html too (needs makeinfo)
echo   -x                   build the cross compiler too
echo   -clean               delete all previously produced files and directories
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
if not %T%_==_ goto :p2
set T=32
if %PROCESSOR_ARCHITECTURE%_==AMD64_ set T=64
if %PROCESSOR_ARCHITEW6432%_==AMD64_ set T=64
:p2
if "%CC:~-3%"=="gcc" set CC=%CC% -O2 -s -static
if (%BINDIR%)==() set BINDIR=%TCCDIR%

set D32=-DTCC_TARGET_PE -DTCC_TARGET_I386
set D64=-DTCC_TARGET_PE -DTCC_TARGET_X86_64
set P32=i386-win32
set P64=x86_64-win32

if %T%==64 goto :t64
set D=%D32%
set P=%P32%
set DX=%D64%
set PX=%P64%
set TX=64
goto :p3

:t64
set D=%D64%
set P=%P64%
set DX=%D32%
set PX=%P32%
set TX=32
goto :p3

:p3
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
if not (%GITHASH%)==() echo>> ..\config.h #define TCC_GITHASH "%GITHASH%"
@if not (%BINDIR%)==(%TCCDIR%) echo>> ..\config.h #define CONFIG_TCCDIR "%TCCDIR:\=/%"
if %TX%==64 echo>> ..\config.h #ifdef TCC_TARGET_X86_64
if %TX%==32 echo>> ..\config.h #ifdef TCC_TARGET_I386
echo>> ..\config.h #define CONFIG_TCC_CROSSPREFIX "%PX%-"
echo>> ..\config.h #endif

for %%f in (*tcc.exe *tcc.dll) do @del %%f

@if _%TCC_C%_==__ goto compiler_2parts
@rem if TCC_C was defined then build only tcc.exe
%CC% -o tcc.exe %TCC_C% %D%
@if errorlevel 1 goto :the_end
@goto :compiler_done

:compiler_2parts
@if _%LIBTCC_C%_==__ set LIBTCC_C=..\libtcc.c
%CC% -o libtcc.dll -shared %LIBTCC_C% %D% -DLIBTCC_AS_DLL
@if errorlevel 1 goto :the_end
%CC% -o tcc.exe ..\tcc.c libtcc.dll %D% -DONE_SOURCE"=0"
@if errorlevel 1 goto :the_end
if not _%XCC%_==_yes_ goto :compiler_done
%CC% -o %PX%-tcc.exe ..\tcc.c %DX%
@if errorlevel 1 goto :the_end
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
call :make_lib %T% || goto :the_end
@if exist %PX%-tcc.exe call :make_lib %TX% %PX%- || goto :the_end

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
.\tcc -B. -m%1 -c ../lib/libtcc1.c
.\tcc -B. -m%1 -c lib/crt1.c
.\tcc -B. -m%1 -c lib/crt1w.c
.\tcc -B. -m%1 -c lib/wincrt1.c
.\tcc -B. -m%1 -c lib/wincrt1w.c
.\tcc -B. -m%1 -c lib/dllcrt1.c
.\tcc -B. -m%1 -c lib/dllmain.c
.\tcc -B. -m%1 -c lib/chkstk.S
.\tcc -B. -m%1 -c ../lib/alloca.S
.\tcc -B. -m%1 -c ../lib/alloca-bt.S
.\tcc -B. -m%1 -c ../lib/stdatomic.c
.\tcc -B. -m%1 -ar lib/%2libtcc1.a libtcc1.o crt1.o crt1w.o wincrt1.o wincrt1w.o dllcrt1.o dllmain.o chkstk.o alloca.o alloca-bt.o stdatomic.o
.\tcc -B. -m%1 -c ../lib/bcheck.c -o lib/%2bcheck.o -bt
.\tcc -B. -m%1 -c ../lib/bt-exe.c -o lib/%2bt-exe.o
.\tcc -B. -m%1 -c ../lib/bt-log.c -o lib/%2bt-log.o
.\tcc -B. -m%1 -c ../lib/bt-dll.c -o lib/%2bt-dll.o
.\tcc -B. -m%1 -c ../lib/runmain.c -o lib/%2runmain.o
exit /B %ERRORLEVEL%
