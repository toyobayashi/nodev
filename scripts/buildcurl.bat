@echo off

call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsamd64_x86.bat"
cd deps\curl\winbuild
nmake /f Makefile.vc mode=static VC=16 MACHINE=x86 ENABLE_WINSSL=yes RTLIBCFG=static
REM mode=dll/static RTLIBCFG=static
REM need define CURL_STATICLIB
cd ..\..\..
