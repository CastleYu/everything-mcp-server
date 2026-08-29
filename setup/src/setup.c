//
// Everything 1.5 Official-Style MCP Plugin Setup Launcher
//

#define _WIN32_WINNT 0x0600
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>
#include "../res/resource.h"

#define SETUP_MAX_STRING                        4096
#define SETUP_MAX_COMMAND_LINE                  32768

#define SETUP_NAME                              "MCP 服务"
#define SETUP_DESCRIPTION                       "允许 AI 助手 (Claude, Cursor, Cline) 通过 MCP 协议搜索文件并访问内容"
#define SETUP_AUTHOR                            "CastleYu"
#define SETUP_VERSION                           "1.0.1.0"
#define SETUP_LINK                              "https://github.com/CastleYu/everything-mcp-server"
#define SETUP_DLL_NAME                          "mcp_server64.dll"

#define SETUP_EVERYTHING_PROGRAM_NAME           "Everything"
#define SETUP_EVERYTHING_PROGRAM_NAME_1_5_ALPHA "Everything (1.5a)"
#define SETUP_EVERYTHING_TASKBAR_NOTIFICATION   "EVERYTHING_TASKBAR_NOTIFICATION"
#define SETUP_EVERYTHING_TASKBAR_NOTIFICATION_1_5_ALPHA "EVERYTHING_TASKBAR_NOTIFICATION_(1.5a)"

static void setup_fatal(DWORD error_code, const wchar_t *msg)
{
    wchar_t title[SETUP_MAX_STRING];
    wchar_t full_msg[SETUP_MAX_STRING];

    wsprintfW(title, L"Setup Everything Plug-in");
    if (error_code != 0)
    {
        wsprintfW(full_msg, L"Error %lu: %s", error_code, msg);
    }
    else
    {
        wsprintfW(full_msg, L"%s", msg);
    }

    MessageBoxW(NULL, full_msg, title, MB_ICONERROR | MB_OK);
    ExitProcess(1);
}

static void *setup_alloc(DWORD size)
{
    void *p = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
    if (!p)
    {
        setup_fatal(ERROR_OUTOFMEMORY, L"Out of memory");
    }
    return p;
}

static void setup_free(void *ptr)
{
    if (ptr)
    {
        HeapFree(GetProcessHeap(), 0, ptr);
    }
}

static int setup_get_reg_string(HKEY root_hkey, const wchar_t *key, const wchar_t *value, wchar_t *buf)
{
    int ret = 0;
    HKEY hkey;

    if (RegOpenKeyExW(root_hkey, key, 0, KEY_READ, &hkey) == ERROR_SUCCESS)
    {
        DWORD buf_size = SETUP_MAX_STRING * sizeof(wchar_t);
        if (RegQueryValueExW(hkey, value, 0, NULL, (BYTE *)buf, &buf_size) == ERROR_SUCCESS)
        {
            DWORD buf_wlen = buf_size / sizeof(wchar_t);
            if (buf_wlen > 0 && buf[buf_wlen - 1] == 0)
            {
                ret = 1;
            }
        }
        RegCloseKey(hkey);
    }
    return ret;
}

static int setup_get_running_exe_filename(const wchar_t *window_class, wchar_t *everything_exe_filename_wbuf)
{
    int ret = 0;
    HWND hwnd = FindWindowW(window_class, NULL);
    if (hwnd)
    {
        DWORD process_id = 0;
        if (GetWindowThreadProcessId(hwnd, &process_id) && process_id != 0)
        {
            HANDLE process_handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, process_id);
            if (process_handle)
            {
                if (GetModuleFileNameExW(process_handle, NULL, everything_exe_filename_wbuf, SETUP_MAX_STRING))
                {
                    ret = 1;
                }
                CloseHandle(process_handle);
            }
        }
    }
    return ret;
}

static int setup_browse_for_exe(wchar_t *everything_exe_filename_wbuf)
{
    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(OPENFILENAMEW));
    ofn.lStructSize = sizeof(OPENFILENAMEW);
    ofn.lpstrFilter = L"Everything.exe\0Everything*.exe\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = everything_exe_filename_wbuf;
    ofn.nMaxFile = SETUP_MAX_STRING;
    ofn.lpstrTitle = L"Select Everything.exe Location";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;

    everything_exe_filename_wbuf[0] = L'\0';

    if (GetOpenFileNameW(&ofn))
    {
        return 1;
    }
    return 0;
}

static int setup_get_everything_exe_filename(wchar_t *everything_exe_filename_wbuf)
{
    // 1. Check running 1.5a window
    if (setup_get_running_exe_filename(L"EVERYTHING_TASKBAR_NOTIFICATION_(1.5a)", everything_exe_filename_wbuf))
    {
        return 1;
    }

    // 2. Check running 1.4 window
    if (setup_get_running_exe_filename(L"EVERYTHING_TASKBAR_NOTIFICATION", everything_exe_filename_wbuf))
    {
        return 1;
    }

    // 3. Check registry for 1.5a
    if (setup_get_reg_string(HKEY_LOCAL_MACHINE, L"SOFTWARE\\voidtools\\Everything (1.5a)", L"ExePath", everything_exe_filename_wbuf))
    {
        if (GetFileAttributesW(everything_exe_filename_wbuf) != INVALID_FILE_ATTRIBUTES) return 1;
    }
    if (setup_get_reg_string(HKEY_CURRENT_USER, L"SOFTWARE\\voidtools\\Everything (1.5a)", L"ExePath", everything_exe_filename_wbuf))
    {
        if (GetFileAttributesW(everything_exe_filename_wbuf) != INVALID_FILE_ATTRIBUTES) return 1;
    }

    // 4. Check registry for standard Everything
    if (setup_get_reg_string(HKEY_LOCAL_MACHINE, L"SOFTWARE\\voidtools\\Everything", L"ExePath", everything_exe_filename_wbuf))
    {
        if (GetFileAttributesW(everything_exe_filename_wbuf) != INVALID_FILE_ATTRIBUTES) return 1;
    }
    if (setup_get_reg_string(HKEY_CURRENT_USER, L"SOFTWARE\\voidtools\\Everything", L"ExePath", everything_exe_filename_wbuf))
    {
        if (GetFileAttributesW(everything_exe_filename_wbuf) != INVALID_FILE_ATTRIBUTES) return 1;
    }

    // 5. Check common paths
    const wchar_t *common_paths[] = {
        L"C:\\Program Files\\Everything 1.5a\\Everything64.exe",
        L"C:\\Program Files\\Everything\\Everything.exe",
        L"C:\\Program Files\\Everything\\Everything64.exe",
        L"D:\\Set\\Everything\\Everything\\Everything.exe",
        L"D:\\Set\\Everything\\Everything64.exe"
    };
    for (size_t i = 0; i < sizeof(common_paths)/sizeof(common_paths[0]); i++)
    {
        if (GetFileAttributesW(common_paths[i]) != INVALID_FILE_ATTRIBUTES)
        {
            lstrcpyW(everything_exe_filename_wbuf, common_paths[i]);
            return 1;
        }
    }

    // 6. User browse dialog
    if (setup_browse_for_exe(everything_exe_filename_wbuf))
    {
        return 1;
    }

    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    (void)hInstance;
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    OleInitialize(NULL);

    wchar_t everything_exe[SETUP_MAX_STRING];
    if (!setup_get_everything_exe_filename(everything_exe))
    {
        setup_fatal(ERROR_FILE_NOT_FOUND, L"无法找到 Everything.exe。请确保 Everything 1.5 正在运行或已安装。");
        return 1;
    }

    wchar_t setup_exe[SETUP_MAX_STRING];
    if (!GetModuleFileNameW(NULL, setup_exe, SETUP_MAX_STRING))
    {
        setup_fatal(GetLastError(), L"无法获取安装程序路径。");
        return 1;
    }

    wchar_t *cmd_line = (wchar_t *)setup_alloc(SETUP_MAX_COMMAND_LINE * sizeof(wchar_t));
    if (!cmd_line) return 1;

    // Build the official -setup-plugin command line
    wsprintfW(cmd_line,
        L"\"%s\" -setup-plugin \"%s\" -setup-plugin-exe \"%s\" -setup-plugin-resource-id \"%d\" -setup-plugin-name \"%s\" -setup-plugin-description \"%s\" -setup-plugin-author \"%s\" -setup-plugin-version \"%s\" -setup-plugin-link \"%s\"",
        everything_exe,
        L"mcp_server64.dll",
        setup_exe,
        IDR_DLL_BZ2,
        L"MCP 服务",
        L"允许 AI 助手 (Claude, Cursor, Cline) 通过 MCP 协议搜索文件并访问内容",
        L"CastleYu",
        L"1.0.1.0",
        L"https://github.com/CastleYu/everything-mcp-server"
    );

    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(STARTUPINFOW));
    si.cb = sizeof(STARTUPINFOW);
    ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

    if (CreateProcessW(everything_exe, cmd_line, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
    {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
    else
    {
        setup_fatal(GetLastError(), L"无法启动 Everything 进行插件安装。");
    }

    setup_free(cmd_line);
    OleUninitialize();
    return 0;
}

int main(void)
{
    return wWinMain(GetModuleHandle(NULL), NULL, NULL, SW_SHOWDEFAULT);
}
