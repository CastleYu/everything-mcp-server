@echo off
setlocal
echo ========================================================
echo Building Everything MCP Setup Installer (Official Style)
echo ========================================================

set SCRIPT_DIR=%~dp0
set OUTPUT_DIR=%SCRIPT_DIR%..\dist-plugin
set RES_DIR=%SCRIPT_DIR%res
set SRC_DIR=%SCRIPT_DIR%src

if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"
if not exist "%RES_DIR%" mkdir "%RES_DIR%"

if not exist "%OUTPUT_DIR%\mcp_server64.dll" (
    echo [ERROR] %OUTPUT_DIR%\mcp_server64.dll not found! Please build the DLL first.
    exit /b 1
)

if exist "%RES_DIR%\setup.dll.bz2" del /f /q "%RES_DIR%\setup.dll.bz2"

echo Compressing mcp_server64.dll to setup.dll.bz2...
where 7z >nul 2>nul
if %ERRORLEVEL% equ 0 (
    7z a -tbzip2 "%RES_DIR%\setup.dll.bz2" "%OUTPUT_DIR%\mcp_server64.dll" -mx=9 >nul
) else if exist "D:\Set\7zip\7-Zip\7z.exe" (
    "D:\Set\7zip\7-Zip\7z.exe" a -tbzip2 "%RES_DIR%\setup.dll.bz2" "%OUTPUT_DIR%\mcp_server64.dll" -mx=9 >nul
) else if exist "C:\Program Files\7-Zip\7z.exe" (
    "C:\Program Files\7-Zip\7z.exe" a -tbzip2 "%RES_DIR%\setup.dll.bz2" "%OUTPUT_DIR%\mcp_server64.dll" -mx=9 >nul
) else (
    echo Warning: 7z not found, copying raw DLL as setup.dll.bz2
    copy /y "%OUTPUT_DIR%\mcp_server64.dll" "%RES_DIR%\setup.dll.bz2" >nul
)

echo Compiling setup resources with windres...
pushd "%RES_DIR%"
windres -i "setup.rc" -O coff -o "setup.res.o"
set WINDRES_ERR=%ERRORLEVEL%
popd

if %WINDRES_ERR% neq 0 (
    echo [ERROR] Resource compilation failed with error code %WINDRES_ERR%
    exit /b %WINDRES_ERR%
)

echo Compiling setup launcher executable...
gcc -std=c99 -O2 -Wall -Wextra -mwindows ^
    -o "%OUTPUT_DIR%\Everything-MCP-Server-Setup.exe" ^
    "%SRC_DIR%\setup.c" ^
    "%RES_DIR%\setup.res.o" ^
    -lkernel32 -luser32 -lshell32 -lole32 -lcomdlg32 -lpsapi

if %ERRORLEVEL% equ 0 (
    echo [SUCCESS] Built %OUTPUT_DIR%\Everything-MCP-Server-Setup.exe successfully!
) else (
    echo [ERROR] Setup build failed with error code %ERRORLEVEL%
    exit /b %ERRORLEVEL%
)
