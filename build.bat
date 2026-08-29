@echo off
setlocal
echo ========================================================
echo Building Everything 1.5 Native MCP Plugin ^& Setup Installer
echo ========================================================

call "%~dp0native-plugin\build.bat"
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%

call "%~dp0setup\build-setup.bat"
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%

echo ========================================================
echo All Components Built Successfully in dist-plugin\
echo 1. dist-plugin\mcp_server64.dll
echo 2. dist-plugin\Everything-MCP-Server-Setup.exe
echo ========================================================
