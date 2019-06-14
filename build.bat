@echo off
if not exist build (mkdir build)
pushd build
REM CALL "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
cl -Zi ..\src\main.c ..\src\gecko_bglib.c ..\src\uart_win.c ..\src\app.c ..\src\discovery.c /I "..\include"
popd
