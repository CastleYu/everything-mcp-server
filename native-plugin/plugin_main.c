//
// Everything 1.5 Native Model Context Protocol (MCP) Server Plugin
// Reference architecture: voidtools http_server & etp_server
//

#define _WIN32_IE 0x0501
#define _WIN32_WINNT 0x0501
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "everything_plugin.h"
#include "version.h"
#include "mcp_server.h"

// Options Page Control IDs
enum
{
	MCP_SERVER_ID_ENABLED_CHECKBOX = 1000,
	MCP_SERVER_ID_PIPE_STATIC,
	MCP_SERVER_ID_PIPE_EDIT,
	MCP_SERVER_ID_PORT_STATIC,
	MCP_SERVER_ID_PORT_EDIT,
	MCP_SERVER_ID_ALLOW_PREVIEW_CHECKBOX,
	MCP_SERVER_ID_MAX_RESULTS_STATIC,
	MCP_SERVER_ID_MAX_RESULTS_EDIT,
	MCP_SERVER_ID_RESTORE_DEFAULTS_BUTTON
};

// Global settings
int g_mcp_enabled = 1;
char g_mcp_pipe_name[256] = "\\\\.\\pipe\\EverythingMCP";
int g_mcp_http_port = 8765;
int g_mcp_allow_preview = 1;
int g_mcp_max_results = 100;

// Host Everything DB pointer
void *g_everything_db = NULL;

// Everything Host function pointers
void* (*everything_plugin_mem_alloc)(uintptr_t size) = NULL;
void* (*everything_plugin_mem_calloc)(uintptr_t size) = NULL;
void (*everything_plugin_mem_free)(void *ptr) = NULL;

everything_plugin_db_t* (*everything_plugin_db_add_local_ref)(void) = NULL;
void (*everything_plugin_db_release)(everything_plugin_db_t *db) = NULL;

everything_plugin_db_query_t* (*everything_plugin_db_query_create)(everything_plugin_db_t *db, void *event_proc, void *user_data) = NULL;
void (*everything_plugin_db_query_destroy)(everything_plugin_db_query_t *q) = NULL;
int (*everything_plugin_db_query_search)(everything_plugin_db_query_t *q, int match_case, int match_whole_word, int match_path, int match_diacritics, int match_prefix, int match_suffix, int ignore_punctuation, int ignore_whitespace, int match_regex, int hide_empty_search_results, int clear_selection, int clear_item_refs, const everything_plugin_utf8_t *search_string, int fast_sort_only, const everything_plugin_property_t *sort_property_type, int sort_ascending, const everything_plugin_property_t *sort_property_type2, int sort_ascending2, const everything_plugin_property_t *sort_property_type3, int sort_ascending3, int folders_first, int track_selected_and_total_file_size, int track_selected_folder_size, int force, int allow_query_access, int allow_read_access, int allow_disk_access, int hide_omit_results, int size_standard, int sort_mix) = NULL;
uintptr_t (*everything_plugin_db_query_get_result_count)(const everything_plugin_db_query_t *q) = NULL;
void (*everything_plugin_db_query_get_result_name)(everything_plugin_db_query_t *q, uintptr_t index, everything_plugin_utf8_buf_t *cbuf) = NULL;
void (*everything_plugin_db_query_get_result_path)(everything_plugin_db_query_t *q, uintptr_t index, everything_plugin_utf8_buf_t *cbuf) = NULL;
void (*everything_plugin_db_query_get_result_indexed_fd)(everything_plugin_db_query_t *q, uintptr_t index, everything_plugin_fileinfo_fd_t *fd) = NULL;
int (*everything_plugin_db_query_is_folder_result)(everything_plugin_db_query_t *q, uintptr_t index) = NULL;

void (*everything_plugin_utf8_buf_init)(everything_plugin_utf8_buf_t *cbuf) = NULL;
void (*everything_plugin_utf8_buf_kill)(everything_plugin_utf8_buf_t *cbuf) = NULL;
everything_plugin_utf8_t* (*everything_plugin_utf8_string_alloc_utf8_string)(const everything_plugin_utf8_t *s) = NULL;

int (*everything_plugin_get_setting_int)(void *data, const everything_plugin_utf8_t *name, int default_value) = NULL;
everything_plugin_utf8_t* (*everything_plugin_get_setting_string)(void *data, const everything_plugin_utf8_t *name, everything_plugin_utf8_t *current_string) = NULL;
void (*everything_plugin_set_setting_int)(void *data, const everything_plugin_utf8_t *name, int value) = NULL;
void (*everything_plugin_set_setting_string)(void *data, const everything_plugin_utf8_t *name, const everything_plugin_utf8_t *value) = NULL;

void* (*everything_plugin_ui_options_add_plugin_page)(void *add_custom_page, void *user_data, const everything_plugin_utf8_t *name) = NULL;
HWND (*everything_plugin_os_create_checkbox)(HWND parent, int id, DWORD extra_style, int checked, const everything_plugin_utf8_t *text) = NULL;
HWND (*everything_plugin_os_create_static)(HWND parent, int id, DWORD extra_window_style, const everything_plugin_utf8_t *text) = NULL;
HWND (*everything_plugin_os_create_edit)(HWND parent, int id, DWORD extra_style, const everything_plugin_utf8_t *text) = NULL;
HWND (*everything_plugin_os_create_number_edit)(HWND parent, int id, DWORD extra_style, __int64 number) = NULL;
HWND (*everything_plugin_os_create_button)(HWND parent, int id, DWORD extra_window_style, const everything_plugin_utf8_t *text) = NULL;
void (*everything_plugin_os_add_tooltip)(HWND tooltip, HWND parent, int id, const everything_plugin_utf8_t *text) = NULL;
void (*everything_plugin_os_set_dlg_rect)(HWND parent_hwnd, int id, int x, int y, int wide, int high) = NULL;
int (*everything_plugin_os_set_dlg_text)(HWND hDlg, int nIDDlgItem, const everything_plugin_utf8_t *s) = NULL;
void (*everything_plugin_os_get_dlg_text)(HWND hwnd, int id, everything_plugin_utf8_buf_t *cbuf) = NULL;
void (*everything_plugin_os_enable_or_disable_dlg_item)(HWND parent_hwnd, int id, int enable) = NULL;
int (*everything_plugin_os_get_logical_wide)(void) = NULL;
int (*everything_plugin_os_get_logical_high)(void) = NULL;
int (*everything_plugin_os_expand_dialog_text_logical_wide_no_prefix)(HWND parent, const everything_plugin_utf8_t *text, int wide) = NULL;

typedef struct mcp_server_everything_plugin_proc_s
{
	const char *name;
	void **proc_address_ptr;
} mcp_server_everything_plugin_proc_t;

static mcp_server_everything_plugin_proc_t mcp_server_proc_array[] =
{
	{"mem_alloc", (void *)&everything_plugin_mem_alloc},
	{"mem_calloc", (void *)&everything_plugin_mem_calloc},
	{"mem_free", (void *)&everything_plugin_mem_free},
	{"db_add_local_ref", (void *)&everything_plugin_db_add_local_ref},
	{"db_release", (void *)&everything_plugin_db_release},
	{"db_query_create", (void *)&everything_plugin_db_query_create},
	{"db_query_destroy", (void *)&everything_plugin_db_query_destroy},
	{"db_query_search", (void *)&everything_plugin_db_query_search},
	{"db_query_get_result_count", (void *)&everything_plugin_db_query_get_result_count},
	{"db_query_get_result_name", (void *)&everything_plugin_db_query_get_result_name},
	{"db_query_get_result_path", (void *)&everything_plugin_db_query_get_result_path},
	{"db_query_get_result_indexed_fd", (void *)&everything_plugin_db_query_get_result_indexed_fd},
	{"db_query_is_folder_result", (void *)&everything_plugin_db_query_is_folder_result},
	{"utf8_buf_init", (void *)&everything_plugin_utf8_buf_init},
	{"utf8_buf_kill", (void *)&everything_plugin_utf8_buf_kill},
	{"utf8_string_alloc_utf8_string", (void *)&everything_plugin_utf8_string_alloc_utf8_string},
	{"plugin_get_setting_int", (void *)&everything_plugin_get_setting_int},
	{"plugin_get_setting_string", (void *)&everything_plugin_get_setting_string},
	{"plugin_set_setting_int", (void *)&everything_plugin_set_setting_int},
	{"plugin_set_setting_string", (void *)&everything_plugin_set_setting_string},
	{"ui_options_add_plugin_page", (void *)&everything_plugin_ui_options_add_plugin_page},
	{"os_create_checkbox", (void *)&everything_plugin_os_create_checkbox},
	{"os_create_static", (void *)&everything_plugin_os_create_static},
	{"os_create_edit", (void *)&everything_plugin_os_create_edit},
	{"os_create_number_edit", (void *)&everything_plugin_os_create_number_edit},
	{"os_create_button", (void *)&everything_plugin_os_create_button},
	{"os_add_tooltip", (void *)&everything_plugin_os_add_tooltip},
	{"os_set_dlg_rect", (void *)&everything_plugin_os_set_dlg_rect},
	{"os_set_dlg_text", (void *)&everything_plugin_os_set_dlg_text},
	{"os_get_dlg_text", (void *)&everything_plugin_os_get_dlg_text},
	{"os_enable_or_disable_dlg_item", (void *)&everything_plugin_os_enable_or_disable_dlg_item},
	{"os_get_logical_wide", (void *)&everything_plugin_os_get_logical_wide},
	{"os_get_logical_high", (void *)&everything_plugin_os_get_logical_high},
	{"os_expand_dialog_text_logical_wide_no_prefix", (void *)&everything_plugin_os_expand_dialog_text_logical_wide_no_prefix},
};

#define MCP_SERVER_PROC_COUNT (sizeof(mcp_server_proc_array) / sizeof(mcp_server_everything_plugin_proc_t))

static void mcp_server_update_options_page(HWND page_hwnd)
{
	int is_enabled = (IsDlgButtonChecked(page_hwnd, MCP_SERVER_ID_ENABLED_CHECKBOX) == BST_CHECKED) ? 1 : 0;

	if (everything_plugin_os_enable_or_disable_dlg_item)
	{
		everything_plugin_os_enable_or_disable_dlg_item(page_hwnd, MCP_SERVER_ID_PIPE_STATIC, is_enabled);
		everything_plugin_os_enable_or_disable_dlg_item(page_hwnd, MCP_SERVER_ID_PIPE_EDIT, is_enabled);
		everything_plugin_os_enable_or_disable_dlg_item(page_hwnd, MCP_SERVER_ID_PORT_STATIC, is_enabled);
		everything_plugin_os_enable_or_disable_dlg_item(page_hwnd, MCP_SERVER_ID_PORT_EDIT, is_enabled);
		everything_plugin_os_enable_or_disable_dlg_item(page_hwnd, MCP_SERVER_ID_ALLOW_PREVIEW_CHECKBOX, is_enabled);
		everything_plugin_os_enable_or_disable_dlg_item(page_hwnd, MCP_SERVER_ID_MAX_RESULTS_STATIC, is_enabled);
		everything_plugin_os_enable_or_disable_dlg_item(page_hwnd, MCP_SERVER_ID_MAX_RESULTS_EDIT, is_enabled);
	}
}

// Plugin Export Function
__declspec(dllexport) void * EVERYTHING_PLUGIN_API everything_plugin_proc(DWORD msg, void *data)
{
	switch (msg)
	{
		case EVERYTHING_PLUGIN_PM_INIT:
		{
			everything_plugin_get_proc_address_t get_proc = (everything_plugin_get_proc_address_t)data;
			if (!get_proc) return (void *)0;

			for (uintptr_t i = 0; i < MCP_SERVER_PROC_COUNT; i++)
			{
				void *proc = get_proc((const everything_plugin_utf8_t *)mcp_server_proc_array[i].name);
				if (proc)
				{
					*mcp_server_proc_array[i].proc_address_ptr = proc;
				}
			}

			if (everything_plugin_db_add_local_ref)
			{
				g_everything_db = everything_plugin_db_add_local_ref();
			}

			mcp_server_init(g_everything_db);
			return (void *)1;
		}

		case EVERYTHING_PLUGIN_PM_GET_PLUGIN_VERSION:
			return (void *)EVERYTHING_PLUGIN_VERSION;

		case EVERYTHING_PLUGIN_PM_GET_NAME:
			return (void *)"Model Context Protocol (MCP) Server";

		case EVERYTHING_PLUGIN_PM_GET_DESCRIPTION:
			return (void *)"Allow AI assistants (Claude, Cursor, Cline, Windsurf) to search files and preview content via MCP.";

		case EVERYTHING_PLUGIN_PM_GET_AUTHOR:
			return (void *)"CastleYu";

		case EVERYTHING_PLUGIN_PM_GET_VERSION:
			return (void *)PLUGINVERSION;

		case EVERYTHING_PLUGIN_PM_GET_LINK:
			return (void *)"https://github.com/CastleYu/everything-mcp-server";

		case EVERYTHING_PLUGIN_PM_START:
		{
			if (everything_plugin_get_setting_int)
			{
				g_mcp_enabled = everything_plugin_get_setting_int(data, (const everything_plugin_utf8_t *)"mcp_enabled", g_mcp_enabled);
				g_mcp_http_port = everything_plugin_get_setting_int(data, (const everything_plugin_utf8_t *)"mcp_http_port", g_mcp_http_port);
				g_mcp_allow_preview = everything_plugin_get_setting_int(data, (const everything_plugin_utf8_t *)"mcp_allow_preview", g_mcp_allow_preview);
				g_mcp_max_results = everything_plugin_get_setting_int(data, (const everything_plugin_utf8_t *)"mcp_max_results", g_mcp_max_results);
			}

			if (everything_plugin_get_setting_string)
			{
				everything_plugin_utf8_t *pipe_str = everything_plugin_get_setting_string(data, (const everything_plugin_utf8_t *)"mcp_pipe_name", NULL);
				if (pipe_str && *pipe_str)
				{
					strncpy(g_mcp_pipe_name, (const char *)pipe_str, sizeof(g_mcp_pipe_name) - 1);
					g_mcp_pipe_name[sizeof(g_mcp_pipe_name) - 1] = '\0';
				}
			}

			mcp_server_apply_settings();
			return (void *)1;
		}

		case EVERYTHING_PLUGIN_PM_STOP:
		{
			mcp_server_shutdown();
			return (void *)1;
		}

		case EVERYTHING_PLUGIN_PM_KILL:
		{
			mcp_server_destroy();
			if (g_everything_db && everything_plugin_db_release)
			{
				everything_plugin_db_release(g_everything_db);
				g_everything_db = NULL;
			}
			return (void *)1;
		}

		case EVERYTHING_PLUGIN_PM_ADD_OPTIONS_PAGES:
		{
			if (everything_plugin_ui_options_add_plugin_page)
			{
				everything_plugin_ui_options_add_plugin_page(data, NULL, (const everything_plugin_utf8_t *)"MCP Server");
			}
			return (void *)1;
		}

		case EVERYTHING_PLUGIN_PM_LOAD_OPTIONS_PAGE:
		{
			everything_plugin_load_options_page_t *load_page = (everything_plugin_load_options_page_t *)data;
			if (!load_page) return (void *)0;

			HWND page_hwnd = load_page->page_hwnd;
			HWND tooltip_hwnd = load_page->tooltip_hwnd;

			if (everything_plugin_os_create_checkbox)
			{
				everything_plugin_os_create_checkbox(page_hwnd, MCP_SERVER_ID_ENABLED_CHECKBOX, WS_GROUP, g_mcp_enabled, (const everything_plugin_utf8_t *)"Enable MCP Server");
				everything_plugin_os_add_tooltip(tooltip_hwnd, page_hwnd, MCP_SERVER_ID_ENABLED_CHECKBOX, (const everything_plugin_utf8_t *)"Enable or disable the Model Context Protocol (MCP) server");

				everything_plugin_os_create_static(page_hwnd, MCP_SERVER_ID_PIPE_STATIC, SS_LEFTNOWORDWRAP | WS_GROUP, (const everything_plugin_utf8_t *)"Named Pipe Name:");
				everything_plugin_os_create_edit(page_hwnd, MCP_SERVER_ID_PIPE_EDIT, WS_GROUP, (const everything_plugin_utf8_t *)g_mcp_pipe_name);
				everything_plugin_os_add_tooltip(tooltip_hwnd, page_hwnd, MCP_SERVER_ID_PIPE_EDIT, (const everything_plugin_utf8_t *)"Windows Named Pipe path for local MCP clients");

				everything_plugin_os_create_static(page_hwnd, MCP_SERVER_ID_PORT_STATIC, SS_LEFTNOWORDWRAP | WS_GROUP, (const everything_plugin_utf8_t *)"HTTP / SSE Port:");
				everything_plugin_os_create_number_edit(page_hwnd, MCP_SERVER_ID_PORT_EDIT, WS_GROUP, g_mcp_http_port);
				everything_plugin_os_add_tooltip(tooltip_hwnd, page_hwnd, MCP_SERVER_ID_PORT_EDIT, (const everything_plugin_utf8_t *)"Port for HTTP / Server-Sent Events (SSE) MCP transport");

				everything_plugin_os_create_checkbox(page_hwnd, MCP_SERVER_ID_ALLOW_PREVIEW_CHECKBOX, WS_GROUP, g_mcp_allow_preview, (const everything_plugin_utf8_t *)"Allow file content preview (everything_preview_file)");
				everything_plugin_os_add_tooltip(tooltip_hwnd, page_hwnd, MCP_SERVER_ID_ALLOW_PREVIEW_CHECKBOX, (const everything_plugin_utf8_t *)"Allow AI assistants to read preview lines of indexed files");

				everything_plugin_os_create_static(page_hwnd, MCP_SERVER_ID_MAX_RESULTS_STATIC, SS_LEFTNOWORDWRAP | WS_GROUP, (const everything_plugin_utf8_t *)"Default Max Results:");
				everything_plugin_os_create_number_edit(page_hwnd, MCP_SERVER_ID_MAX_RESULTS_EDIT, WS_GROUP, g_mcp_max_results);
				everything_plugin_os_add_tooltip(tooltip_hwnd, page_hwnd, MCP_SERVER_ID_MAX_RESULTS_EDIT, (const everything_plugin_utf8_t *)"Default limit for search results returned to AI assistants");

				everything_plugin_os_create_button(page_hwnd, MCP_SERVER_ID_RESTORE_DEFAULTS_BUTTON, WS_GROUP, (const everything_plugin_utf8_t *)"Restore Defaults");
				everything_plugin_os_add_tooltip(tooltip_hwnd, page_hwnd, MCP_SERVER_ID_RESTORE_DEFAULTS_BUTTON, (const everything_plugin_utf8_t *)"Reset MCP Server settings to default values");

				mcp_server_update_options_page(page_hwnd);
			}

			return (void *)1;
		}

		case EVERYTHING_PLUGIN_PM_SAVE_OPTIONS_PAGE:
		{
			everything_plugin_save_options_page_t *save_page = (everything_plugin_save_options_page_t *)data;
			if (!save_page) return (void *)0;

			HWND page_hwnd = save_page->page_hwnd;

			g_mcp_enabled = (IsDlgButtonChecked(page_hwnd, MCP_SERVER_ID_ENABLED_CHECKBOX) == BST_CHECKED);
			g_mcp_allow_preview = (IsDlgButtonChecked(page_hwnd, MCP_SERVER_ID_ALLOW_PREVIEW_CHECKBOX) == BST_CHECKED);

			if (everything_plugin_os_get_dlg_text)
			{
				everything_plugin_utf8_buf_t pipe_buf;
				everything_plugin_utf8_buf_init(&pipe_buf);
				everything_plugin_os_get_dlg_text(page_hwnd, MCP_SERVER_ID_PIPE_EDIT, &pipe_buf);
				if (pipe_buf.buf && *pipe_buf.buf)
				{
					strncpy(g_mcp_pipe_name, (const char *)pipe_buf.buf, sizeof(g_mcp_pipe_name) - 1);
					g_mcp_pipe_name[sizeof(g_mcp_pipe_name) - 1] = '\0';
				}
				everything_plugin_utf8_buf_kill(&pipe_buf);
			}

			g_mcp_http_port = GetDlgItemInt(page_hwnd, MCP_SERVER_ID_PORT_EDIT, NULL, FALSE);
			if (g_mcp_http_port <= 0 || g_mcp_http_port > 65535) g_mcp_http_port = 8765;

			g_mcp_max_results = GetDlgItemInt(page_hwnd, MCP_SERVER_ID_MAX_RESULTS_EDIT, NULL, FALSE);
			if (g_mcp_max_results <= 0) g_mcp_max_results = 100;

			mcp_server_apply_settings();
			return (void *)1;
		}

		case EVERYTHING_PLUGIN_PM_GET_OPTIONS_PAGE_MINMAX:
		{
			everything_plugin_get_options_page_minmax_t *minmax = (everything_plugin_get_options_page_minmax_t *)data;
			if (minmax)
			{
				minmax->wide = 240;
				minmax->high = 260;
			}
			return (void *)1;
		}

		case EVERYTHING_PLUGIN_PM_SIZE_OPTIONS_PAGE:
		{
			everything_plugin_size_options_page_t *size_page = (everything_plugin_size_options_page_t *)data;
			if (!size_page || !everything_plugin_os_set_dlg_rect) return (void *)0;

			HWND page_hwnd = size_page->page_hwnd;
			RECT rect;
			GetClientRect(page_hwnd, &rect);
			int wide = rect.right - rect.left;
			int high = rect.bottom - rect.top;

			if (everything_plugin_os_get_logical_wide && everything_plugin_os_get_logical_high)
			{
				int log_w = everything_plugin_os_get_logical_wide();
				int log_h = everything_plugin_os_get_logical_high();
				if (log_w > 0) wide = (wide * 96) / log_w;
				if (log_h > 0) high = (high * 96) / log_h;
			}

			int x = 12;
			int y = 12;
			wide -= 24;
			int static_w = 120;

			everything_plugin_os_set_dlg_rect(page_hwnd, MCP_SERVER_ID_ENABLED_CHECKBOX, x, y, wide, 18);
			y += 24;

			everything_plugin_os_set_dlg_rect(page_hwnd, MCP_SERVER_ID_PIPE_STATIC, x, y + 2, static_w, 16);
			everything_plugin_os_set_dlg_rect(page_hwnd, MCP_SERVER_ID_PIPE_EDIT, x + static_w, y, wide - static_w, 21);
			y += 27;

			everything_plugin_os_set_dlg_rect(page_hwnd, MCP_SERVER_ID_PORT_STATIC, x, y + 2, static_w, 16);
			everything_plugin_os_set_dlg_rect(page_hwnd, MCP_SERVER_ID_PORT_EDIT, x + static_w, y, 80, 21);
			y += 27;

			everything_plugin_os_set_dlg_rect(page_hwnd, MCP_SERVER_ID_ALLOW_PREVIEW_CHECKBOX, x, y, wide, 18);
			y += 24;

			everything_plugin_os_set_dlg_rect(page_hwnd, MCP_SERVER_ID_MAX_RESULTS_STATIC, x, y + 2, static_w, 16);
			everything_plugin_os_set_dlg_rect(page_hwnd, MCP_SERVER_ID_MAX_RESULTS_EDIT, x + static_w, y, 80, 21);
			y += 32;

			int button_w = 110;
			everything_plugin_os_set_dlg_rect(page_hwnd, MCP_SERVER_ID_RESTORE_DEFAULTS_BUTTON, x + wide - button_w, y, button_w, 24);

			return (void *)1;
		}

		case EVERYTHING_PLUGIN_PM_OPTIONS_PAGE_PROC:
		{
			everything_plugin_options_page_proc_t *page_proc = (everything_plugin_options_page_proc_t *)data;
			if (page_proc && page_proc->msg == WM_COMMAND)
			{
				int control_id = LOWORD(page_proc->wParam);
				HWND page_hwnd = page_proc->page_hwnd;

				if (control_id == MCP_SERVER_ID_ENABLED_CHECKBOX)
				{
					mcp_server_update_options_page(page_hwnd);
				}
				else if (control_id == MCP_SERVER_ID_RESTORE_DEFAULTS_BUTTON)
				{
					CheckDlgButton(page_hwnd, MCP_SERVER_ID_ENABLED_CHECKBOX, BST_CHECKED);
					CheckDlgButton(page_hwnd, MCP_SERVER_ID_ALLOW_PREVIEW_CHECKBOX, BST_CHECKED);
					if (everything_plugin_os_set_dlg_text)
					{
						everything_plugin_os_set_dlg_text(page_hwnd, MCP_SERVER_ID_PIPE_EDIT, (const everything_plugin_utf8_t *)"\\\\.\\pipe\\EverythingMCP");
					}
					SetDlgItemInt(page_hwnd, MCP_SERVER_ID_PORT_EDIT, 8765, FALSE);
					SetDlgItemInt(page_hwnd, MCP_SERVER_ID_MAX_RESULTS_EDIT, 100, FALSE);
					mcp_server_update_options_page(page_hwnd);
				}
			}
			return (void *)1;
		}

		case EVERYTHING_PLUGIN_PM_SAVE_SETTINGS:
		{
			if (everything_plugin_set_setting_int)
			{
				everything_plugin_set_setting_int(data, (const everything_plugin_utf8_t *)"mcp_enabled", g_mcp_enabled);
				everything_plugin_set_setting_int(data, (const everything_plugin_utf8_t *)"mcp_http_port", g_mcp_http_port);
				everything_plugin_set_setting_int(data, (const everything_plugin_utf8_t *)"mcp_allow_preview", g_mcp_allow_preview);
				everything_plugin_set_setting_int(data, (const everything_plugin_utf8_t *)"mcp_max_results", g_mcp_max_results);
			}
			if (everything_plugin_set_setting_string)
			{
				everything_plugin_set_setting_string(data, (const everything_plugin_utf8_t *)"mcp_pipe_name", (const everything_plugin_utf8_t *)g_mcp_pipe_name);
			}
			return (void *)1;
		}

		default:
			break;
	}

	return (void *)0;
}
