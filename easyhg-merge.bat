@echo off
setlocal enableextensions
setlocal enabledelayedexpansion
set mypath=%~dp0
set kdiff=kdiff3.exe
set found=
set SEARCH=%mypath%;%PATH%
:loop
for /F "delims=; tokens=1*" %%e in ("%SEARCH%") do (
    if exist "%%e\%kdiff%" (
        set found=%%e\%kdiff%
        goto done;
    )
    set SEARCH=%%f
)
if not "%SEARCH%"=="" goto loop;
:done
if "%found%"=="" (
    echo. "Failed to find kdiff.exe in path"
) else (
    "%found%" "%~2" "%~1" "%~3" -o "%~1" --auto -L1 "%~1 (Common ancestor)" -L2 "%~1 (Your current version)" -L3 "%~1 (Version being merged)"
)
