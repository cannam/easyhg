rem  Run this from within the top-level project dir: deploy\win32\build.bat

rem  NB Sip build command, in sip-4.19.13 directory: c:\Python27\python.exe configure.py --sip-module PyQt5.sip --no-tools
rem  NB PyQt5 build command, in PyQt5_gpl-5.11.3 directory: c:\Python27\python.exe configure.py --qmake=c:\qt\5.12.0\msvc2017\bin\qmake.exe --sip=c:\Python27\sip.exe --disable=QtNfc

set STARTPWD=%CD%

if not exist "C:\Program Files (x86)\WiX Toolset v3.11\bin" (
@   echo Could not find WiX Toolset
@   exit /b 2
)

if not exist "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Redist\MSVC\14.15.26706\x86\Microsoft.VC141.CRT" (
@   echo Could not find Windows CRT directory
@   exit /b 2
)

if not exist "C:/Python27/Lib/site-packages/PyQt5/" (
@   echo Could not find PyQt5 directory
@   exit /b 2
)

if not exist "C:/Python27/Lib/site-packages/Crypto/" (
@   echo Could not find PyCrypto directory
@   exit /b 2
)

set ORIGINALPATH=%PATH%
set PATH=C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Bin;%PATH%
set CRTDIR=C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Redist\MSVC\14.15.26706\x86\Microsoft.VC141.CRT
set PYPACKAGEDIR=C:/Python27/Lib/site-packages
set PYQTDIR=%PYPACKAGEDIR%/PyQt5/
set PYCRYPTODIR=%PYPACKAGEDIR%/Crypto/
set NAME=Open Source Developer, Christopher Cannam

set ARG=%1
shift
if "%ARG%" == "sign" (
@   echo NOTE: sign option specified, will attempt to codesign exe and msi
) else (
@   echo NOTE: sign option not specified, will not codesign anything
)

cd %STARTPWD%
del /q /s build_win32
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

copy "%STARTPWD%\easyhg.py" .

copy "%CRTDIR%\concrt140.DLL" .
copy "%CRTDIR%\msvcp140.DLL" .
copy "%CRTDIR%\vccorlib140.DLL" .
copy "%CRTDIR%\vcruntime140.DLL" .

copy "%HGDIR%\PFiles\Mercurial\hg.exe" .
copy "%HGDIR%\PFiles\Mercurial\python27.dll" .

mkdir lib
copy "%HGDIR%\PFiles\Mercurial\lib\*" lib\

copy "%HGDIR%\windows\system32\msvcm90.dll" .
copy "%HGDIR%\windows\system32\msvcp90.dll" .
copy "%HGDIR%\windows\system32\msvcr90.dll" .

mkdir PyQt5
copy "%PYQTDIR%\__init__.py" PyQt5\__init__.py
copy "%PYQTDIR%\Qt.pyd" PyQt5\Qt.pyd
copy "%PYQTDIR%\QtCore.pyd" PyQt5\QtCore.pyd
copy "%PYQTDIR%\QtGui.pyd" PyQt5\QtGui.pyd
copy "%PYQTDIR%\QtWidgets.pyd" PyQt5\QtWidgets.pyd
copy "%PYQTDIR%\sip.pyd" PyQt5\sip.pyd

mkdir enum
copy "%PYPACKAGEDIR%\enum\__init__.py" enum\__init__.py

mkdir Crypto
mkdir Crypto\Cipher
mkdir Crypto\Util

copy "%PYCRYPTODIR%\Cipher\__init__.py" Crypto\Cipher\
copy "%PYCRYPTODIR%\Cipher\_AES.pyd" Crypto\Cipher\
copy "%PYCRYPTODIR%\Cipher\AES.py" Crypto\Cipher\
copy "%PYCRYPTODIR%\Cipher\blockalgo.py" Crypto\Cipher\

copy "%PYCRYPTODIR%\Util\__init__.py" Crypto\Util\
copy "%PYCRYPTODIR%\Util\py3compat.py" Crypto\Util\

copy "%PYCRYPTODIR%\__init__.py" Crypto\

if "%ARG%" == "sign" (
@echo Signing components
signtool sign /v /n "%NAME%" /t http://time.certum.pl /fd sha1 *.dll *.exe Crypto\Cipher\*.pyd PyQt5\*.pyd lib\*.pyd
)

set PATH=%PATH%;"C:\Program Files (x86)\WiX Toolset v3.11\bin"

del easyhg.msi
candle -v ..\..\deploy\win32\easyhg.wxs
light -b . -ext WixUIExtension -ext WixUtilExtension -v easyhg.wixobj
if %errorlevel% neq 0 exit /b %errorlevel%

if "%ARG%" == "sign" (
@echo Signing package
signtool sign /v /n "%NAME%" /t http://time.certum.pl /fd sha1 easyhg.msi
signtool verify /pa easyhg.msi
)

