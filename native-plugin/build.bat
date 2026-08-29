@echo off
setlocal

echo ======================================================
echo Building Everything 1.5 Native MCP Plugin (64-bit DLL)
echo ======================================================

if not exist "..\dist-plugin" mkdir "..\dist-plugin"

gcc -std=c99 -shared -O3 -Wall -Wextra -o "..\dist-plugin\mcp_server64.dll" plugin_main.c mcp_server.c -lkernel32 -luser32 -Wl,--out-implib,..\dist-plugin\libmcp_server64.a

if %ERRORLEVEL% equ 0 (
    echo [SUCCESS] Successfully compiled mcp_server64.dll
    echo [INFO] Location: dist-plugin\mcp_server64.dll
) else (
    echo [ERROR] Build failed with error code %ERRORLEVEL%
)

endlocal
