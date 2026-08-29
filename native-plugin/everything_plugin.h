#ifndef EVERYTHING_PLUGIN_H
#define EVERYTHING_PLUGIN_H

#include <windows.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EVERYTHING_PLUGIN_API __stdcall

#define EVERYTHING_PLUGIN_VERSION 1

// Plugin Message (PM) IDs
#define EVERYTHING_PLUGIN_PM_INIT                     1
#define EVERYTHING_PLUGIN_PM_GET_PLUGIN_VERSION       2
#define EVERYTHING_PLUGIN_PM_GET_NAME                 3
#define EVERYTHING_PLUGIN_PM_GET_DESCRIPTION          4
#define EVERYTHING_PLUGIN_PM_GET_AUTHOR               5
#define EVERYTHING_PLUGIN_PM_GET_VERSION              6
#define EVERYTHING_PLUGIN_PM_GET_LINK                 7
#define EVERYTHING_PLUGIN_PM_START                    8
#define EVERYTHING_PLUGIN_PM_STOP                     9
#define EVERYTHING_PLUGIN_PM_KILL                     10
#define EVERYTHING_PLUGIN_PM_UNINSTALL                11
#define EVERYTHING_PLUGIN_PM_ADD_OPTIONS_PAGES        12
#define EVERYTHING_PLUGIN_PM_LOAD_OPTIONS_PAGE        13
#define EVERYTHING_PLUGIN_PM_SAVE_OPTIONS_PAGE        14
#define EVERYTHING_PLUGIN_PM_GET_OPTIONS_PAGE_MINMAX  15
#define EVERYTHING_PLUGIN_PM_SIZE_OPTIONS_PAGE        16
#define EVERYTHING_PLUGIN_PM_OPTIONS_PAGE_PROC        17
#define EVERYTHING_PLUGIN_PM_KILL_OPTIONS_PAGE        18
#define EVERYTHING_PLUGIN_PM_SAVE_SETTINGS            19

// Query sort flags
#define EVERYTHING_PLUGIN_SORT_NAME_ASCENDING         1
#define EVERYTHING_PLUGIN_SORT_NAME_DESCENDING        2
#define EVERYTHING_PLUGIN_SORT_PATH_ASCENDING         3
#define EVERYTHING_PLUGIN_SORT_PATH_DESCENDING        4
#define EVERYTHING_PLUGIN_SORT_SIZE_ASCENDING         5
#define EVERYTHING_PLUGIN_SORT_SIZE_DESCENDING        6
#define EVERYTHING_PLUGIN_SORT_EXTENSION_ASCENDING    7
#define EVERYTHING_PLUGIN_SORT_EXTENSION_DESCENDING   8
#define EVERYTHING_PLUGIN_SORT_DATE_MODIFIED_ASCENDING 9
#define EVERYTHING_PLUGIN_SORT_DATE_MODIFIED_DESCENDING 10
#define EVERYTHING_PLUGIN_SORT_DATE_CREATED_ASCENDING 11
#define EVERYTHING_PLUGIN_SORT_DATE_CREATED_DESCENDING 12
#define EVERYTHING_PLUGIN_SORT_ATTRIBUTES_ASCENDING   13
#define EVERYTHING_PLUGIN_SORT_ATTRIBUTES_DESCENDING  14

// Opaque types
typedef struct everything_plugin_db_query_s everything_plugin_db_query_t;
typedef struct everything_plugin_utf8_buf_s everything_plugin_utf8_buf_t;

// UTF-8 Buffer structure
struct everything_plugin_utf8_buf_s {
    char *data;
    size_t length;
    size_t allocated;
};

// Everything Host API Table passed in EVERYTHING_PLUGIN_PM_INIT
typedef struct everything_plugin_api_s {
    // Memory
    void *(__stdcall *mem_alloc)(size_t size);
    void *(__stdcall *mem_calloc)(size_t num, size_t size);
    void (__stdcall *mem_free)(void *ptr);

    // Database Queries
    everything_plugin_db_query_t *(__stdcall *db_query_create)(void);
    void (__stdcall *db_query_destroy)(everything_plugin_db_query_t *q);
    int (__stdcall *db_query_search)(everything_plugin_db_query_t *q, const char *search_utf8, uint32_t flags);
    uint32_t (__stdcall *db_query_get_result_count)(everything_plugin_db_query_t *q);
    const char *(__stdcall *db_query_get_result_name)(everything_plugin_db_query_t *q, uint32_t index);
    const char *(__stdcall *db_query_get_result_path)(everything_plugin_db_query_t *q, uint32_t index);
    int (__stdcall *db_query_is_folder_result)(everything_plugin_db_query_t *q, uint32_t index);
    void (__stdcall *db_query_sort)(everything_plugin_db_query_t *q, uint32_t sort_type, int ascending);
    void (__stdcall *db_query_cancel)(everything_plugin_db_query_t *q);

    // Settings (Plugins.ini)
    int (__stdcall *plugin_get_setting_int)(const wchar_t *name, int default_value);
    void (__stdcall *plugin_set_setting_int)(const wchar_t *name, int value);
    const wchar_t *(__stdcall *plugin_get_setting_string)(const wchar_t *name, const wchar_t *default_value);
    void (__stdcall *plugin_set_setting_string)(const wchar_t *name, const wchar_t *value);

    // Logging & Diagnostics
    void (__cdecl *debug_printf)(const char *format, ...);
    void (__cdecl *debug_error_printf)(const char *format, ...);

    // UTF-8 Helpers
    void (__stdcall *utf8_buf_init)(everything_plugin_utf8_buf_t *buf);
    void (__stdcall *utf8_buf_kill)(everything_plugin_utf8_buf_t *buf);
    void (__stdcall *utf8_buf_copy_utf8_string)(everything_plugin_utf8_buf_t *buf, const char *str);
    void (__stdcall *utf8_buf_cat_utf8_string)(everything_plugin_utf8_buf_t *buf, const char *str);
} everything_plugin_api_t;

// Main entry point exported by plugin DLL
__declspec(dllexport) void * EVERYTHING_PLUGIN_API everything_plugin_proc(DWORD msg, void *data);

#ifdef __cplusplus
}
#endif

#endif // EVERYTHING_PLUGIN_H
