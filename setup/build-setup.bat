@echo off
setlocal
echo ========================================================
echo Building Everything MCP Setup Installer (Setup.exe)
echo ========================================================

set SCRIPT_DIR=%~dp0
set OUTPUT_DIR=%SCRIPT_DIR%..\dist-plugin

if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"

gcc -std=c99 -O2 -Wall -Wextra -mwindows ^
    -o "%OUTPUT_DIR%\Everything-MCP-Server-Setup.exe" ^
    "%SCRIPT_DIR%src\setup.c" ^
    -lkernel32 -luser32 -lshell32 -lole32

if %ERRORLEVEL% equ 0 (
    echo [SUCCESS] Built %OUTPUT_DIR%\Everything-MCP-Server-Setup.exe successfully!
) else (
    echo [ERROR] Setup build failed with error code %ERRORLEVEL%
    exit /b %ERRORLEVEL%
)
