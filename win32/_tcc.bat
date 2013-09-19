@echo off
setlocal enabledelayedexpansion

pushd %~dp0

path %~dp0;%path%

set EXT=.exe
echo %*|findstr /R /C:"\<-c\>" >nul &&set EXT=.o
echo %*|findstr /R /C:"\<-shared\>" >nul &&set EXT=.dll

::1st file found must be the main c file to get output file name
set OUTF=
call :FINDFN %*

if "%OUTF%"=="" goto :EXIT

call _parseLibs -vv -o "%OUTF%" %*

:EXIT
popd
pause
exit /b

:FINDFN
for %%i in (%*) do (
    if exist %%i set OUTF=%%~dpni%EXT%&goto :ENDFDF
)
:ENDFDF
exit /b
