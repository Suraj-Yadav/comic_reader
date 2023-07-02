@echo off

set CACHE=

SETLOCAL ENABLEEXTENSIONS
IF ERRORLEVEL 1 ECHO Unable to enable extensions
IF DEFINED VCPKG_INSTALLATION_ROOT (
    set CMAKE_TOOLCHAIN_FILE=%VCPKG_INSTALLATION_ROOT%\scripts\buildsystems\vcpkg.cmake 
    set CACHE=--binarysource=clear;x-gha,readwrite
)

vcpkg install --triplet x64-windows tweeny wxwidgets freeimage libarchive[core] %CACHE%


cmake -B build -S . -GNinja -DCMAKE_BUILD_TYPE=Release || rmdir /S /Q build && mkdir build 
cmake -B build -S . -GNinja -DCMAKE_BUILD_TYPE=Release
if %errorlevel% neq 0 exit /b %errorlevel%

cmake --build build --target package
if %errorlevel% neq 0 exit /b %errorlevel%

ENDLOCAL
