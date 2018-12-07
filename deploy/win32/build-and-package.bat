rem  Run this from within the top-level project dir: deploy\win32\build.bat

set STARTPWD=%CD%

if not exist "C:\Program Files (x86)\WiX Toolset v3.11\bin" (
@   echo Could not find WiX Toolset
@   exit /b 2
)

if not exist "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Redist\MSVC\14.15.26706\x64\Microsoft.VC141.CRT" (
@   echo Could not find Windows CRT directory %CRTDIR%
@   exit /b 2
)

set ORIGINALPATH=%PATH%
set PATH=C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Bin;%PATH%
set CRTDIR=C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Redist\MSVC\14.15.26706\x64\Microsoft.VC141.CRT
set NAME=Open Source Developer, Christopher Cannam

set ARG=%1
shift
if "%ARG%" == "sign" (
@   echo NOTE: sign option specified, will attempt to codesign exe and msi
) else (
@   echo NOTE: sign option not specified, will not codesign anything
)

cd %STARTPWD%
rem del /q /s build_win32
call .\deploy\win32\build.bat
if %errorlevel% neq 0 exit /b %errorlevel%

if not exist mercurial-4.8.0-x86.msi (
     C:\ProgramData\chocolatey\bin\wget https://bitbucket.org/tortoisehg/files/downloads/mercurial-4.8.0-x86.msi
)
if %errorlevel% neq 0 exit /b %errorlevel%

if not exist mercurial-4.8.0-x86 (
     msiexec /a mercurial-4.8.0-x86.msi /qn TARGETDIR=%STARTPWD%\mercurial-4.8.0-x86
)
if %errorlevel% neq 0 exit /b %errorlevel%

set HGDIR=%STARTPWD%\mercurial-4.8.0-x86

cd build_win32\release

copy "%CRTDIR%\concrt140.DLL" .
copy "%CRTDIR%\msvcp140.DLL" .
copy "%CRTDIR%\vccorlib140.DLL" .
copy "%CRTDIR%\vcruntime140.DLL" .

copy "%HGDIR%\PFiles\Mercurial\hg.exe" .
copy "%HGDIR%\PFiles\Mercurial\python27.dll" .

mkdir lib
copy "%HGDIR%\PFiles\Mercurial\lib\*" .\lib\

copy "%HGDIR%\windows\system32\msvcm90.dll" .
copy "%HGDIR%\windows\system32\msvcp90.dll" .
copy "%HGDIR%\windows\system32\msvcr90.dll" .

if "%ARG%" == "sign" (
@echo Signing components
signtool sign /v /n "%NAME%" /t http://time.certum.pl /fd sha1 *.dll *.exe lib\*
signtool verify /pa sonic-visualiser.msi
)

rem Todo: the rest
