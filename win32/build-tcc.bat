@rem ------------------------------------------------------
@rem batch file to build tcc using mingw, msvc or tcc itself
@rem ------------------------------------------------------

@echo off
setlocal
if (%1)==(-clean) goto :cleanup
set CC=gcc
set /p VERSION= < ..\VERSION
git.exe --version 2>nul
if not %ERRORLEVEL%==0 goto :git_done
for /f %%b in ('git.exe rev-parse --abbrev-ref HEAD') do set GITHASH=%%b
for /f %%h in ('git.exe rev-parse --short HEAD') do set GITHASH=%GITHASH%:%%h
git.exe diff --quiet
if %ERRORLEVEL%==1 set GITHASH=%GITHASH%-mod
set DEF_GITHASH=-DTCC_GITHASH="""%GITHASH%"""
:git_done
set INST=
set DOC=no
set EXES_ONLY=no
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
if (%1)==(-i) set INST=%2&& goto :a2
if (%1)==(-d) set DOC=yes&& goto :a3
if (%1)==(-x) set EXES_ONLY=yes&& goto :a3
if (%1)==() goto :p1
:usage
echo usage: build-tcc.bat [ options ... ]
echo options:
echo   -c prog              use prog (gcc/tcc/cl) to compile tcc
echo   -c "prog options"    use prog with options to compile tcc
echo   -t 32/64             force 32/64 bit default target
echo   -v "version"         set tcc version
echo   -i tccdir            install tcc into tccdir
echo   -d                   create tcc-doc.html too (needs makeinfo)
echo   -x                   just create the executables
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
if "%CC:~-3%"=="gcc" set CC=%CC% -O2 -s -static %DEF_GITHASH%
set D32=-DTCC_TARGET_PE -DTCC_TARGET_I386
set D64=-DTCC_TARGET_PE -DTCC_TARGET_X86_64
set P32=i386-win32
set P64=x86_64-win32
if %T%==64 goto :t64
set D=%D32%
set DX=%D64%
set PX=%P64%
set TX=64
goto :p3
:t64
set D=%D64%
set DX=%D32%
set PX=%P32%
set TX=32
goto :p3

:p3
@echo on

:config.h
echo>..\config.h #define TCC_VERSION "%VERSION%"
echo>> ..\config.h #ifdef TCC_TARGET_X86_64
echo>> ..\config.h #define TCC_LIBTCC1 "libtcc1-64.a"
echo>> ..\config.h #else
echo>> ..\config.h #define TCC_LIBTCC1 "libtcc1-32.a"
echo>> ..\config.h #endif

for %%f in (*tcc.exe *tcc.dll) do @del %%f

@if _%TCC_C%_==__ goto compiler_2parts
@rem if TCC_C was defined then build only tcc.exe
%CC% -o tcc.exe %TCC_C% %D%
@goto :compiler_done

:compiler_2parts
@if _%LIBTCC_C%_==__ set LIBTCC_C=..\libtcc.c
%CC% -o libtcc.dll -shared %LIBTCC_C% %D% -DLIBTCC_AS_DLL
@if errorlevel 1 goto :the_end
%CC% -o tcc.exe ..\tcc.c libtcc.dll %D% -DONE_SOURCE"=0"
%CC% -o %PX%-tcc.exe ..\tcc.c %DX%
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

:libtcc1.a
call :makelib %T%
@if errorlevel 1 goto :the_end
@if exist %PX%-tcc.exe call :makelib %TX%
@if errorlevel 1 goto :the_end
.\tcc -m%T% -c ../lib/bcheck.c -o lib/bcheck.o -g
.\tcc -m%T% -c ../lib/bt-exe.c -o lib/bt-exe.o
.\tcc -m%T% -c ../lib/bt-log.c -o lib/bt-log.o
.\tcc -m%T% -c ../lib/bt-dll.c -o lib/bt-dll.o

:tcc-doc.html
@if not (%DOC%)==(yes) goto :doc-done
echo>..\config.texi @set VERSION %VERSION%
cmd /c makeinfo --html --no-split ../tcc-doc.texi -o doc/tcc-doc.html
:doc-done

:files_done
for %%f in (*.o *.def) do @del %%f

:copy-install
@if (%INST%)==() goto :the_end
if not exist %INST% mkdir %INST%
for %%f in (*tcc.exe *tcc.dll) do @copy>nul %%f %INST%\%%f
@if not exist %INST%\lib mkdir %INST%\lib
for %%f in (lib\*.a lib\*.o lib\*.def) do @copy>nul %%f %INST%\%%f
for %%f in (include examples libtcc doc) do @xcopy>nul /s/i/q/y %%f %INST%\%%f

:the_end
exit /B %ERRORLEVEL%

:makelib
.\tcc -m%1 -c ../lib/libtcc1.c
.\tcc -m%1 -c lib/crt1.c
.\tcc -m%1 -c lib/crt1w.c
.\tcc -m%1 -c lib/wincrt1.c
.\tcc -m%1 -c lib/wincrt1w.c
.\tcc -m%1 -c lib/dllcrt1.c
.\tcc -m%1 -c lib/dllmain.c
.\tcc -m%1 -c lib/chkstk.S
.\tcc -m%1 -c ../lib/alloca.S
.\tcc -m%1 -c ../lib/alloca-bt.S
.\tcc -m%1 -c ../lib/stdatomic.c
.\tcc -m%1 -ar lib/libtcc1-%1.a libtcc1.o crt1.o crt1w.o wincrt1.o wincrt1w.o dllcrt1.o dllmain.o chkstk.o alloca.o alloca-bt.o stdatomic.o
exit /B %ERRORLEVEL%
