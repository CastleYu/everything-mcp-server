#ifndef MCP_SERVER_H
#define MCP_SERVER_H

#include "everything_plugin.h"

#ifdef __cplusplus
extern "C" {
#endif

// Initialize the MCP server subsystem
int mcp_server_init(everything_plugin_api_t *api);

// Start the MCP Named Pipe / IPC listener thread
int mcp_server_start(const wchar_t *pipe_name, int tcp_port);

// Stop and join the worker threads
int mcp_server_stop(void);

// Process a single JSON-RPC request line and produce a JSON-RPC response line
char *mcp_process_json_rpc(const char *request_json, size_t request_len);

// Free response memory allocated by mcp_process_json_rpc
void mcp_free_response(char *response);

#ifdef __cplusplus
}
#endif

#endif // MCP_SERVER_H
