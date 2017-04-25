@rem ------------------------------------------------------
@rem batch file to build tcc using mingw, msvc or tcc itself
@rem ------------------------------------------------------

@echo off
setlocal

set CC=gcc -Os -s -static
set /p VERSION= < ..\VERSION
set INST=
set DOC=no
goto :a0

:usage
echo usage: build-tcc.bat [ options ... ]
echo options:
echo   -c prog              use prog (gcc/tcc/cl) to compile tcc
echo   -c "prog options"    use prog with options to compile tcc
echo   -t 32/64             force 32/64 bit default target
echo   -v "version"         set tcc version
echo   -i dir               install tcc into dir
echo   -d                   create tcc-doc.html too (needs makeinfo)
exit /B 1

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
%CMD% -O1 -W2 -Zi -MT -GS- -nologo -link -opt:ref,icf
@exit /B %ERRORLEVEL%

:a2
shift
:a1
shift
:a0
if not (%1)==(-c) goto :a3
set CC=%~2
if (%2)==(cl) set CC=@call :cl
goto :a2
:a3
if not (%1)==(-t) goto :a4
set T=%2
goto :a2
:a4
if not (%1)==(-v) goto :a5
set VERSION=%~2
goto :a2
:a5
if not (%1)==(-i) goto :a6
set INST=%2
goto :a2
:a6
if not (%1)==(-d) goto :a7
set DOC=yes
goto :a1
:a7
if not (%1)==() goto :usage

if not "%CC%"=="@call :cl" goto :p1
set VSCOMNTOOLS=%VS150COMNTOOLS%
if "%VSCOMNTOOLS%"=="" set VSCOMNTOOLS=%VS140COMNTOOLS%
if "%VSCOMNTOOLS%"=="" set VSCOMNTOOLS=%VS130COMNTOOLS%
if "%VSCOMNTOOLS%"=="" set VSCOMNTOOLS=%VS120COMNTOOLS%
if %T%_==32_ set CLVARS="%VSCOMNTOOLS%..\..\VC\bin\vcvars32.bat"
if %T%_==64_ set CLVARS="%VSCOMNTOOLS%..\..\VC\bin\amd64\vcvars64.bat"
if %T%_==_ set T=32& if %Platform%_==X64_ set T=64
if %CLVARS%_==_ goto :p1
if exist %CLVARS% call %CLVARS%
:p1

if not %T%_==_ goto :p2
set T=32
if %PROCESSOR_ARCHITECTURE%_==AMD64_ set T=64
if %PROCESSOR_ARCHITEW6432%_==AMD64_ set T=64
:p2

set D32=-DTCC_TARGET_PE -DTCC_TARGET_I386
set D64=-DTCC_TARGET_PE -DTCC_TARGET_X86_64
set P32=i386-win32
set P64=x86_64-win32
if %T%==64 goto :t64
set D=%D32%
set DX=%D64%
set PX=%P64%
goto :t96
:t64
set D=%D64%
set DX=%D32%
set PX=%P32%
:t96

@echo on

:config.h
echo>..\config.h #define TCC_VERSION "%VERSION%"
echo>> ..\config.h #ifdef TCC_TARGET_X86_64
echo>> ..\config.h #define TCC_LIBTCC1 "libtcc1-64.a"
echo>> ..\config.h #else
echo>> ..\config.h #define TCC_LIBTCC1 "libtcc1-32.a"
echo>> ..\config.h #endif

for %%f in (*tcc.exe *tcc.dll) do @del %%f

:compiler
%CC% -o libtcc.dll -shared ..\libtcc.c %D% -DONE_SOURCE -DLIBTCC_AS_DLL
@if errorlevel 1 goto :the_end
%CC% -o tcc.exe ..\tcc.c libtcc.dll %D%
%CC% -o %PX%-tcc.exe ..\tcc.c %DX% -DONE_SOURCE

@if (%TCC_FILES%)==(no) goto :files-done

if not exist libtcc mkdir libtcc
if not exist doc mkdir doc
copy>nul ..\include\*.h include
copy>nul ..\tcclib.h include
copy>nul ..\libtcc.h libtcc
copy>nul ..\tests\libtcc_test.c examples
copy>nul tcc-win32.txt doc

.\tcc -impdef libtcc.dll -o libtcc\libtcc.def
@if errorlevel 1 goto :the_end

:libtcc1.a
@set O1=libtcc1.o crt1.o crt1w.o wincrt1.o wincrt1w.o dllcrt1.o dllmain.o chkstk.o bcheck.o
.\tcc -m32 %D32% -c ../lib/libtcc1.c
.\tcc -m32 %D32% -c lib/crt1.c
.\tcc -m32 %D32% -c lib/crt1w.c
.\tcc -m32 %D32% -c lib/wincrt1.c
.\tcc -m32 %D32% -c lib/wincrt1w.c
.\tcc -m32 %D32% -c lib/dllcrt1.c
.\tcc -m32 %D32% -c lib/dllmain.c
.\tcc -m32 %D32% -c lib/chkstk.S
.\tcc -m32 %D32% -w -c ../lib/bcheck.c
.\tcc -m32 %D32% -c ../lib/alloca86.S
.\tcc -m32 %D32% -c ../lib/alloca86-bt.S
.\tcc -m32 -ar lib/libtcc1-32.a %O1% alloca86.o alloca86-bt.o
@if errorlevel 1 goto :the_end
.\tcc -m64 %D64% -c ../lib/libtcc1.c
.\tcc -m64 %D64% -c lib/crt1.c
.\tcc -m64 %D64% -c lib/crt1w.c
.\tcc -m64 %D64% -c lib/wincrt1.c
.\tcc -m64 %D64% -c lib/wincrt1w.c
.\tcc -m64 %D64% -c lib/dllcrt1.c
.\tcc -m64 %D64% -c lib/dllmain.c
.\tcc -m64 %D64% -c lib/chkstk.S
.\tcc -m64 %D64% -w -c ../lib/bcheck.c
.\tcc -m64 %D64% -c ../lib/alloca86_64.S
.\tcc -m64 %D64% -c ../lib/alloca86_64-bt.S
.\tcc -m64 -ar lib/libtcc1-64.a %O1% alloca86_64.o alloca86_64-bt.o
@if errorlevel 1 goto :the_end

:tcc-doc.html
@if not (%DOC%)==(yes) goto :doc-done
echo>..\config.texi @set VERSION %VERSION%
cmd /c makeinfo --html --no-split ../tcc-doc.texi -o doc/tcc-doc.html
:doc-done

:files-done
for %%f in (*.o *.def) do @del %%f

:copy-install
@if (%INST%)==() goto :the_end
if not exist %INST% mkdir %INST%
@if not exist %INST%\lib mkdir %INST%\lib
for %%f in (*tcc.exe *tcc.dll lib\*.a lib\*.def) do @copy>nul %%f %INST%\%%f
for %%f in (include examples libtcc doc) do @xcopy>nul /s/i/q/y %%f %INST%\%%f

:the_end
exit /B %ERRORLEVEL%
