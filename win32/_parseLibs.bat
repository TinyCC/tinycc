@echo off
setlocal enabledelayedexpansion

pushd %~dp0

::Define as main parameters
set _Args_=
set _LIBs_=
set LIBi=

set ARGSO=-IExt\include -LExt\lib %*

::This is for the .def file also have a similar name .c file
::.a file will be larger than .def + .c
::*-uuid.c files are suitable to form libuuid.a
::w32api-3.17.2
:GetRLib
for %%i in (%ARGSO%) do (
  set ARG=%%i
  set OPT=!ARG:~0,2!
  if "!OPT!"=="-l" (
    set LIB=!ARG:~2!
    set LIBi=
    if "!LIB!"=="uuid" (
      set LIBi= lib\*uid.c
    ) else (
      if "!LIB!"=="vfw32" (
        set LIBi= lib\msvfw32.def lib\avifil32.def lib\avicap32.def
      ) else (
        call :GetLibS
      )
    )
    if "!LIBi!"=="" (
      set _Args_=!_Args_! %%i
    ) else (
      set LIBi=!LIBi:%~dp0=!
      set _LIBs_=!_LIBs_! !LIBi!
      echo For lib !LIB! will use:
      echo !LIBi!
      echo.
    )
  ) else (
    set _Args_=!_Args_! %%i
  )
)

::GetRLib End
popd

tcc.exe !_Args_! !_LIBs_!

exit /b

::::::::::

:GetLibS
for %%D in (-Llib %ARGSO%) do (
  set ARG_=%%D
  set OPT_=!ARG_:~0,2!
  set LIBD=
  if "!OPT_!"=="-L" (
    set LIBD=!ARG_:~2!
    if exist "!LIBD!" call :GetDLib
  )
)
set LIBD=
set OPT_=
set ARG_=
exit /b
::GetLibD End

:GetDLib
pushd !LIBD!
for /f "usebackq delims=" %%I in (`"dir /b /s !LIB!.c !LIB!_*.c !LIB!.def !LIB!_*.def 2>nul"`) do (
  set LIBi=!LIBi! "%%I"
)
popd
exit /b
::GetDLib End
