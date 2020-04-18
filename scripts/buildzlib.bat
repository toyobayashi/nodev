@echo off

call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsamd64_x86.bat"

REM cd deps\zlib\contrib\vstudio\vc16
REM msbuild zlibvc.sln /t:Rebuild /p:Configuration=Release /p:Platform="Win32"
REM cd ..\..\..\..\..

cd deps\zlib
nmake /f win32\Makefile.msc LOC=-MT
cd ..\..
mkdir lib
copy /Y deps\zlib\zlib.lib  lib\zlib.lib
cd deps\zlib
nmake /f win32\Makefile.msc clean
cd ..\..
