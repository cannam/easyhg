rem  Run this from within the top-level project dir: deploy\win32\build.bat
rem  To build from clean, delete the folder build_win32

set STARTPWD=%CD%

set QTDIR=C:\Qt\5.11.2\mingw53_32
if not exist %QTDIR% (
@   echo Could not find 32-bit Qt
@   exit /b 2
)

set ORIGINALPATH=%PATH%
set PATH=%PATH%;C:\Program Files (x86)\SMLNJ\bin;%QTDIR%\bin;C:\Qt\Tools\QtCreator\bin;C:\Qt\Tools\mingw530_32\bin

cd %STARTPWD%

mkdir build_win32
cd build_win32

qmake -spec win32-g++ -r ..\easyhg.pro
if %errorlevel% neq 0 exit /b %errorlevel%

mingw32-make
if %errorlevel% neq 0 exit /b %errorlevel%

set PATH=%ORIGINALPATH%
