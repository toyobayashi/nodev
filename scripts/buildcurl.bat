@echo off

if /i "%VCVARS_PATH%"=="" (
  call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsamd64_x86.bat"
) else (
  call "%VCVARS_PATH%"
)

set vcversion=16

if defined VCVER (
  set vcversion=%VCVER%
)

cd deps\curl
call buildconf.bat
cd winbuild
nmake /f Makefile.vc mode=static VC=%vcversion% MACHINE=x86 ENABLE_WINSSL=yes RTLIBCFG=static
REM mode=dll/static RTLIBCFG=static
REM need define CURL_STATICLIB
cd ..\..\..

copy /Y "deps\curl\builds\libcurl-vc%vcversion%-x86-release-static-ipv6-sspi-winssl\lib\libcurl_a.lib" lib\libcurl_a.lib
