@echo off
if EXIST "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools" (
    %comspec% /k "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars32.bat"
) else if EXIST "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community" (
    %comspec% /k "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars32.bat"
) else if EXIST "C:\Program Files\Microsoft Visual Studio\2022\BuildTools" (
    %comspec% /k "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars32.bat"
) else if EXIST "C:\Program Files\Microsoft Visual Studio\2022\Community" (
    %comspec% /k "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat"
) else if EXIST "C:\Program Files (x86)\Microsoft Visual Studio\2017\BuildTools" (
    %comspec% /k "C:\Program Files (x86)\Microsoft Visual Studio\2017\BuildTools\VC\Auxiliary\Build\vcvars32.bat"
)
