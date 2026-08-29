#include "everything_plugin.h"
#include "mcp_server.h"
#include <windows.h>

static everything_plugin_api_t *g_api = NULL;
static wchar_t g_pipe_name[256] = L"\\\\.\\pipe\\EverythingMCP";
static int g_enabled = 1;

__declspec(dllexport) void * EVERYTHING_PLUGIN_API everything_plugin_proc(DWORD msg, void *data) {
    switch (msg) {
        case EVERYTHING_PLUGIN_PM_INIT:
            g_api = (everything_plugin_api_t *)data;
            if (g_api) {
                mcp_server_init(g_api);
                return (void *)1;
            }
            return (void *)0;

        case EVERYTHING_PLUGIN_PM_GET_PLUGIN_VERSION:
            return (void *)EVERYTHING_PLUGIN_VERSION;

        case EVERYTHING_PLUGIN_PM_GET_NAME:
            return (void *)L"Model Context Protocol (MCP) Server";

        case EVERYTHING_PLUGIN_PM_GET_DESCRIPTION:
            return (void *)L"Provides in-process native MCP Server for AI assistants (Claude, Cursor, Cline) via Windows Named Pipe with zero port exposure.";

        case EVERYTHING_PLUGIN_PM_GET_AUTHOR:
            return (void *)L"Everything MCP Community";

        case EVERYTHING_PLUGIN_PM_GET_VERSION:
            return (void *)L"1.0.0.1";

        case EVERYTHING_PLUGIN_PM_GET_LINK:
            return (void *)L"https://github.com/CastleYu/everything-mcp-server";

        case EVERYTHING_PLUGIN_PM_START:
            if (g_api && g_api->plugin_get_setting_string) {
                const wchar_t *pipe_setting = g_api->plugin_get_setting_string(L"pipe_name", L"\\\\.\\pipe\\EverythingMCP");
                if (pipe_setting) {
                    wcsncpy(g_pipe_name, pipe_setting, sizeof(g_pipe_name) / sizeof(wchar_t) - 1);
                }
            }
            mcp_server_start(g_pipe_name, 0);
            return (void *)1;

        case EVERYTHING_PLUGIN_PM_STOP:
            mcp_server_stop();
            return (void *)1;

        case EVERYTHING_PLUGIN_PM_KILL:
            mcp_server_stop();
            return (void *)1;

        case EVERYTHING_PLUGIN_PM_SAVE_SETTINGS:
            if (g_api && g_api->plugin_set_setting_string) {
                g_api->plugin_set_setting_string(L"pipe_name", g_pipe_name);
                g_api->plugin_set_setting_int(L"enabled", g_enabled);
            }
            return (void *)1;

        default:
            break;
    }

    return (void *)0;
}

// Standard Windows DLL Entrypoint
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    (void)hinstDLL;
    (void)lpvReserved;

    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            break;
        case DLL_PROCESS_DETACH:
            mcp_server_stop();
            break;
    }
    return TRUE;
}
