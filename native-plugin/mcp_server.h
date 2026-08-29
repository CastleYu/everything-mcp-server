#ifndef _MCP_SERVER_H_
#define _MCP_SERVER_H_

#include <windows.h>
#include <stdint.h>
#include <stdbool.h>
#include "everything_plugin.h"

#ifdef __cplusplus
extern "C" {
#endif

// Server settings & state
extern int g_mcp_enabled;
extern char g_mcp_pipe_name[256];
extern int g_mcp_http_port;
extern int g_mcp_allow_preview;
extern int g_mcp_max_results;

// Host Everything database pointer
extern void *g_everything_db;

// Lifecycle management
int mcp_server_init(void *db);
void mcp_server_apply_settings(void);
void mcp_server_shutdown(void);
void mcp_server_destroy(void);

// Tool execution helpers
char *mcp_execute_search(const char *query, int offset, int max_count, int sort_type, int ascending);
char *mcp_execute_file_info(const char *path);
char *mcp_execute_preview(const char *path, int max_lines, int max_bytes);
char *mcp_execute_status(void);

// JSON-RPC dispatcher
char *mcp_handle_jsonrpc(const char *json_request);

#ifdef __cplusplus
}
#endif

#endif // _MCP_SERVER_H_
