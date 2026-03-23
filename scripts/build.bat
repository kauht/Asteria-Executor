@echo off
rem build.bat - wrapper to initialize MSVC/LLVM environment and run xmake
rem Usage: scripts\build.bat [xmake-args...]
rem This script tries to automatically locate and call Visual Studio's vcvars
rem (or VsDevCmd) or use clang if available, so you don't need to open the
rem Developer Command Prompt manually.

setlocal EnableExtensions EnableDelayedExpansion

rem If cl is already in PATH, assume env is ready
where cl >nul 2>&1
if %ERRORLEVEL%==0 (
    echo [build] MSVC toolchain detected in PATH.
    goto run_xmake
)

rem Try to find vswhere.exe (standard installer location)
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
    for /f "usebackq delims=" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
        set "VSINSTALL=%%I"
    )
)

rem If vswhere found an install, try vcvars64.bat from it
if defined VSINSTALL (
    set "VARS=%VSINSTALL%\VC\Auxiliary\Build\vcvars64.bat"
    if exist "%VARS%" (
        echo [build] Calling vcvars from "%VARS%"
        call "%VARS%" >nul 2>&1
        goto check_cl
    )
    rem fallback to VsDevCmd if present
    set "VDEV=%VSINSTALL%\Common7\Tools\VsDevCmd.bat"
    if exist "%VDEV%" (
        echo [build] Calling VsDevCmd from "%VDEV%"
        call "%VDEV%" -arch=amd64 >nul 2>&1
        goto check_cl
    )
)

rem Try common default install paths (adjust for different VS editions if needed)
set "TRY_VARS=%ProgramFiles%\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
if exist "%TRY_VARS%" (
    echo [build] Calling vcvars from "%TRY_VARS%"
    call "%TRY_VARS%" >nul 2>&1
    goto check_cl
)

set "TRY_VDEV=%ProgramFiles%\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat"
if exist "%TRY_VDEV%" (
    echo [build] Calling VsDevCmd from "%TRY_VDEV%"
    call "%TRY_VDEV%" -arch=amd64 >nul 2>&1
    goto check_cl
)

:check_cl
where cl >nul 2>&1
if %ERRORLEVEL%==0 (
    echo [build] MSVC environment initialized.
    goto run_xmake
)

rem If MSVC isn't available, try clang (may be inside VS tree)
echo [build] MSVC not available in PATH. Trying clang...