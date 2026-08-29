@echo off
setlocal
echo ========================================================
echo Building Everything 1.5 Native MCP Plugin (mcp_server64.dll)
echo ========================================================

set SCRIPT_DIR=%~dp0
set OUTPUT_DIR=%SCRIPT_DIR%..\dist-plugin

if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"

gcc -std=c99 -shared -O3 -Wall -Wextra -D_WIN64 ^
    -o "%OUTPUT_DIR%\mcp_server64.dll" ^
    "%SCRIPT_DIR%plugin_main.c" ^
    "%SCRIPT_DIR%mcp_server.c" ^
    "%SCRIPT_DIR%mcp_server.def" ^
    -lkernel32 -luser32 -lws2_32

if %ERRORLEVEL% equ 0 (
    echo [SUCCESS] Built %OUTPUT_DIR%\mcp_server64.dll successfully!
) else (
    echo [ERROR] Build failed with error code %ERRORLEVEL%
    exit /b %ERRORLEVEL%
)
