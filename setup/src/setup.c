//
// Everything 1.5 MCP Plugin Installer
// Reference: voidtools setup architecture
//

#define _WIN32_IE 0x0501
#define _WIN32_WINNT 0x0501
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <stdio.h>
#include <stdbool.h>

#include "../../native-plugin/version.h"

#define SETUP_NAME          "Model Context Protocol (MCP) Server"
#define SETUP_DESCRIPTION   "Allow AI assistants (Claude, Cursor, Cline) to search files and preview content via MCP."
#define SETUP_AUTHOR        "CastleYu"
#define SETUP_VERSION       PLUGINVERSION
#define SETUP_LINK          "https://github.com/CastleYu/everything-mcp-server"
#define SETUP_DLL_NAME      "mcp_server64.dll"

#define SETUP_MAX_PATH      1024

static bool get_everything_dir(char *out_dir, size_t out_size)
{
	// 1. Try registry HKLM / HKCU
	HKEY hkey;
	const char *reg_keys[] = {
		"SOFTWARE\\voidtools\\Everything",
		"SOFTWARE\\voidtools\\Everything 1.5a",
		"SOFTWARE\\WOW6432Node\\voidtools\\Everything"
	};

	for (size_t i = 0; i < sizeof(reg_keys) / sizeof(reg_keys[0]); i++)
	{
		if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, reg_keys[i], 0, KEY_READ, &hkey) == ERROR_SUCCESS)
		{
			DWORD type = 0;
			DWORD size = (DWORD)out_size;
			if (RegQueryValueExA(hkey, "InstallFolder", NULL, &type, (BYTE *)out_dir, &size) == ERROR_SUCCESS && size > 0)
			{
				RegCloseKey(hkey);
				return true;
			}
			if (RegQueryValueExA(hkey, "AppPath", NULL, &type, (BYTE *)out_dir, &size) == ERROR_SUCCESS && size > 0)
			{
				RegCloseKey(hkey);
				// Strip executable name
				char *slash = strrchr(out_dir, '\\');
				if (slash) *slash = '\0';
				return true;
			}
			RegCloseKey(hkey);
		}

		if (RegOpenKeyExA(HKEY_CURRENT_USER, reg_keys[i], 0, KEY_READ, &hkey) == ERROR_SUCCESS)
		{
			DWORD type = 0;
			DWORD size = (DWORD)out_size;
			if (RegQueryValueExA(hkey, "InstallFolder", NULL, &type, (BYTE *)out_dir, &size) == ERROR_SUCCESS && size > 0)
			{
				RegCloseKey(hkey);
				return true;
			}
			RegCloseKey(hkey);
		}
	}

	// 2. Try standard program files folders
	const char *default_paths[] = {
		"C:\\Program Files\\Everything",
		"C:\\Program Files\\Everything 1.5a",
		"C:\\Program Files (x86)\\Everything"
	};

	for (size_t i = 0; i < sizeof(default_paths) / sizeof(default_paths[0]); i++)
	{
		DWORD attr = GetFileAttributesA(default_paths[i]);
		if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY))
		{
			strncpy(out_dir, default_paths[i], out_size - 1);
			out_dir[out_size - 1] = '\0';
			return true;
		}
	}

	return false;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	(void)hInstance;
	(void)hPrevInstance;
	(void)lpCmdLine;
	(void)nCmdShow;

	char msg[2048];
	snprintf(msg, sizeof(msg),
		"Do you want to install the Everything 1.5 Native MCP Plugin?\n\n"
		"Plugin Name: " SETUP_NAME "\n"
		"Version: " SETUP_VERSION "\n"
		"Author: " SETUP_AUTHOR "\n"
		"Target DLL: " SETUP_DLL_NAME "\n\n"
		"This will install the Model Context Protocol server into Everything 1.5.",
		SETUP_NAME, SETUP_VERSION, SETUP_AUTHOR);

	int res = MessageBoxA(NULL, msg, "Install Everything MCP Plugin", MB_YESNO | MB_ICONQUESTION);
	if (res != IDYES) return 0;

	// Locate source DLL next to Setup.exe or in dist-plugin
	char source_dll[SETUP_MAX_PATH] = {0};
	GetModuleFileNameA(NULL, source_dll, SETUP_MAX_PATH);
	char *last_slash = strrchr(source_dll, '\\');
	if (last_slash) *last_slash = '\0';

	char try_paths[3][SETUP_MAX_PATH];
	snprintf(try_paths[0], SETUP_MAX_PATH, "%s\\%s", source_dll, SETUP_DLL_NAME);
	snprintf(try_paths[1], SETUP_MAX_PATH, "%s\\dist-plugin\\%s", source_dll, SETUP_DLL_NAME);
	snprintf(try_paths[2], SETUP_MAX_PATH, "%s\\..\\dist-plugin\\%s", source_dll, SETUP_DLL_NAME);

	char found_source[SETUP_MAX_PATH] = {0};
	for (int i = 0; i < 3; i++)
	{
		if (GetFileAttributesA(try_paths[i]) != INVALID_FILE_ATTRIBUTES)
		{
			strncpy(found_source, try_paths[i], SETUP_MAX_PATH - 1);
			break;
		}
	}

	if (found_source[0] == '\0')
	{
		MessageBoxA(NULL, "Could not find " SETUP_DLL_NAME " next to the installer.\nPlease ensure " SETUP_DLL_NAME " is present in the same directory.", "Installation Error", MB_OK | MB_ICONERROR);
		return 1;
	}

	char everything_dir[SETUP_MAX_PATH] = {0};
	if (!get_everything_dir(everything_dir, sizeof(everything_dir)))
	{
		// Ask user to browse
		BROWSEINFOA bi = {0};
		bi.lpszTitle = "Select your Everything 1.5 installation directory:";
		bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
		LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
		if (pidl)
		{
			SHGetPathFromIDListA(pidl, everything_dir);
			CoTaskMemFree(pidl);
		}
	}

	if (everything_dir[0] == '\0')
	{
		MessageBoxA(NULL, "Could not find Everything installation directory.", "Installation Aborted", MB_OK | MB_ICONWARNING);
		return 1;
	}

	// Create Plugins directory if not exists
	char plugins_dir[SETUP_MAX_PATH];
	snprintf(plugins_dir, sizeof(plugins_dir), "%s\\Plugins", everything_dir);
	CreateDirectoryA(plugins_dir, NULL);

	char target_dll[SETUP_MAX_PATH];
	snprintf(target_dll, sizeof(target_dll), "%s\\%s", plugins_dir, SETUP_DLL_NAME);

	if (!CopyFileA(found_source, target_dll, FALSE))
	{
		DWORD err = GetLastError();
		char err_msg[512];
		snprintf(err_msg, sizeof(err_msg), "Failed to copy plugin DLL to:\n%s\n\nError Code: %lu\n(If Everything is running, please exit Everything first)", target_dll, err);
		MessageBoxA(NULL, err_msg, "Installation Failed", MB_OK | MB_ICONERROR);
		return 1;
	}

	char success_msg[1024];
	snprintf(success_msg, sizeof(success_msg),
		"Everything MCP Plugin installed successfully!\n\n"
		"Installed to:\n%s\n\n"
		"Please restart Everything 1.5 (File -> Exit, then reopen Everything).\n"
		"The MCP server will be active under Tools -> Options -> Plug-ins -> MCP Server.",
		target_dll);

	MessageBoxA(NULL, success_msg, "Installation Complete", MB_OK | MB_ICONINFORMATION);
	return 0;
}
