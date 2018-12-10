rem  Run this from within the top-level project dir: deploy\win32\build.bat
rem  To build from clean, delete the folder build_win32

set STARTPWD=%CD%

set QTDIR=C:\Qt\5.12.0\msvc2017
if not exist %QTDIR% (
@   echo Could not find 32-bit Qt using MSVC
@   exit /b 2
)

if not exist "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" (
@   echo "Could not find MSVC vars batch file"
@   exit /b 2
)

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x86

set ORIGINALPATH=%PATH%
set PATH=%PATH%;C:\Program Files (x86)\SMLNJ\bin;%QTDIR%\bin;C:\Qt\Tools\QtCreator\bin

cd %STARTPWD%

if not exist easyhg-kdiff3 (
    hg clone https://code.soundsoftware.ac.uk/hg/easyhg-kdiff3
)
set KDIFFDIR=%STARTPWD%\easyhg-kdiff3\kdiff3\src-QT4

mkdir build_win32
cd build_win32

qmake -spec win32-msvc -r -tp vc ..\easyhg.pro
if %errorlevel% neq 0 exit /b %errorlevel%

msbuild EasyMercurial.vcxproj /t:Build /p:Configuration=Release
if %errorlevel% neq 0 exit /b %errorlevel%

cd %KDIFFDIR%

mkdir build_win32
cd build_win32

qmake -spec win32-msvc -r -tp vc ..\kdiff3.pro
if %errorlevel% neq 0 exit /b %errorlevel%

msbuild kdiff3.vcxproj /t:Build /p:Configuration=Release
if %errorlevel% neq 0 exit /b %errorlevel%

cd %STARTPWD%\build_win32

copy %KDIFFDIR%\build_win32\release\kdiff3.exe .\release

copy %QTDIR%\bin\Qt5Core.dll .\release
copy %QTDIR%\bin\Qt5Gui.dll .\release
copy %QTDIR%\bin\Qt5Widgets.dll .\release
copy %QTDIR%\bin\Qt5Network.dll .\release
copy %QTDIR%\bin\Qt5PrintSupport.dll .\release
copy %QTDIR%\plugins\platforms\qminimal.dll .\release
copy %QTDIR%\plugins\platforms\qwindows.dll .\release
copy %QTDIR%\plugins\styles\qwindowsvistastyle.dll .\release

cd %STARTPWD%

set PATH=%ORIGINALPATH%
