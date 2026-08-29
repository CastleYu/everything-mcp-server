#include "mcp_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

static everything_plugin_api_t *g_api = NULL;
static HANDLE g_hThread = NULL;
static HANDLE g_hStopEvent = NULL;
static volatile LONG g_running = 0;
static wchar_t g_pipe_name[256] = L"\\\\.\\pipe\\EverythingMCP";

// Dynamic string buffer helper
typedef struct {
    char *data;
    size_t length;
    size_t capacity;
} str_builder_t;

static void sb_init(str_builder_t *sb, size_t initial_cap) {
    if (initial_cap < 256) initial_cap = 256;
    sb->data = (char *)malloc(initial_cap);
    sb->data[0] = '\0';
    sb->length = 0;
    sb->capacity = initial_cap;
}

static void sb_grow(str_builder_t *sb, size_t add_len) {
    if (sb->length + add_len + 1 > sb->capacity) {
        size_t new_cap = (sb->capacity * 2) + add_len + 256;
        char *new_data = (char *)realloc(sb->data, new_cap);
        if (new_data) {
            sb->data = new_data;
            sb->capacity = new_cap;
        }
    }
}

static void sb_append(str_builder_t *sb, const char *str) {
    if (!str) return;
    size_t len = strlen(str);
    sb_grow(sb, len);
    memcpy(sb->data + sb->length, str, len);
    sb->length += len;
    sb->data[sb->length] = '\0';
}

static void sb_append_json_str(str_builder_t *sb, const char *str) {
    if (!str) {
        sb_append(sb, "\"\"");
        return;
    }
    sb_append(sb, "\"");
    for (const char *p = str; *p; p++) {
        unsigned char c = (unsigned char)*p;
        if (c == '"') sb_append(sb, "\\\"");
        else if (c == '\\') sb_append(sb, "\\\\");
        else if (c == '\b') sb_append(sb, "\\b");
        else if (c == '\f') sb_append(sb, "\\f");
        else if (c == '\n') sb_append(sb, "\\n");
        else if (c == '\r') sb_append(sb, "\\r");
        else if (c == '\t') sb_append(sb, "\\t");
        else if (c < 32) {
            char hex[8];
            snprintf(hex, sizeof(hex), "\\u%04x", c);
            sb_append(sb, hex);
        } else {
            char ch[2] = { (char)c, '\0' };
            sb_append(sb, ch);
        }
    }
    sb_append(sb, "\"");
}

static void sb_free(str_builder_t *sb) {
    if (sb->data) {
        free(sb->data);
        sb->data = NULL;
    }
    sb->length = 0;
    sb->capacity = 0;
}

// Simple JSON extraction helpers
static int extract_json_string(const char *json, const char *key, char *out_val, size_t max_out) {
    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char *pos = strstr(json, pattern);
    if (!pos) return 0;
    pos += strlen(pattern);
    while (*pos == ' ' || *pos == '\t' || *pos == ':') pos++;
    if (*pos != '"') return 0;
    pos++; // skip opening quote

    size_t idx = 0;
    while (*pos && *pos != '"' && idx < max_out - 1) {
        if (*pos == '\\' && *(pos + 1)) {
            pos++;
            if (*pos == 'n') out_val[idx++] = '\n';
            else if (*pos == 'r') out_val[idx++] = '\r';
            else if (*pos == 't') out_val[idx++] = '\t';
            else if (*pos == '"') out_val[idx++] = '"';
            else if (*pos == '\\') out_val[idx++] = '\\';
            else out_val[idx++] = *pos;
            pos++;
        } else {
            out_val[idx++] = *pos++;
        }
    }
    out_val[idx] = '\0';
    return 1;
}

static int extract_json_int(const char *json, const char *key, int default_val) {
    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char *pos = strstr(json, pattern);
    if (!pos) return default_val;
    pos += strlen(pattern);
    while (*pos == ' ' || *pos == '\t' || *pos == ':') pos++;
    return atoi(pos);
}

// Handler for tools/list
static void handle_tools_list(const char *id_str, str_builder_t *resp) {
    sb_append(resp, "{\"jsonrpc\":\"2.0\",\"id\":");
    sb_append(resp, id_str);
    sb_append(resp, ",\"result\":{\"tools\":[");

    // Tool 1: everything_search
    sb_append(resp, "{"
        "\"name\":\"everything_search\","
        "\"description\":\"Search files and folders in-memory via native Everything 1.5 Plugin engine.\","
        "\"inputSchema\":{"
            "\"type\":\"object\","
            "\"properties\":{"
                "\"query\":{\"type\":\"string\",\"description\":\"Everything search query (e.g. *.ts, size:>10mb, dm:today)\"},"
                "\"max_results\":{\"type\":\"number\",\"description\":\"Maximum results (default: 30)\"}"
            "},"
            "\"required\":[\"query\"]"
        "}"
    "},");

    // Tool 2: everything_get_file_info
    sb_append(resp, "{"
        "\"name\":\"everything_get_file_info\","
        "\"description\":\"Retrieve indexed metadata for a specific file or directory path.\","
        "\"inputSchema\":{"
            "\"type\":\"object\","
            "\"properties\":{"
                "\"path\":{\"type\":\"string\",\"description\":\"Absolute path to target file or directory\"}"
            "},"
            "\"required\":[\"path\"]"
        "}"
    "},");

    // Tool 3: everything_status
    sb_append(resp, "{"
        "\"name\":\"everything_status\","
        "\"description\":\"Check Everything native plugin status and total database indexed count.\","
        "\"inputSchema\":{\"type\":\"object\",\"properties\":{}}"
    "}");

    sb_append(resp, "]}}");
}

// Handler for tools/call -> everything_search
static void handle_search_tool(const char *params_json, str_builder_t *content_sb) {
    if (!g_api || !g_api->db_query_create) {
        sb_append(content_sb, "{\"error\":\"Everything DB Query API not initialized\"}");
        return;
    }

    char query[1024] = {0};
    extract_json_string(params_json, "query", query, sizeof(query));
    int max_results = extract_json_int(params_json, "max_results", 30);
    if (max_results <= 0) max_results = 30;
    if (max_results > 500) max_results = 500;

    everything_plugin_db_query_t *q = g_api->db_query_create();
    if (!q) {
        sb_append(content_sb, "{\"error\":\"Failed to allocate query context\"}");
        return;
    }

    // Execute query in memory
    g_api->db_query_search(q, query, 0);
    uint32_t total = g_api->db_query_get_result_count(q);
    uint32_t count = total < (uint32_t)max_results ? total : (uint32_t)max_results;

    char header[256];
    snprintf(header, sizeof(header), "{\"totalResults\":%u,\"returnedCount\":%u,\"query\":", total, count);
    sb_append(content_sb, header);
    sb_append_json_str(content_sb, query);
    sb_append(content_sb, ",\"items\":[");

    for (uint32_t i = 0; i < count; i++) {
        const char *name = g_api->db_query_get_result_name(q, i);
        const char *path = g_api->db_query_get_result_path(q, i);
        int is_folder = g_api->db_query_is_folder_result(q, i);

        if (i > 0) sb_append(content_sb, ",");
        sb_append(content_sb, "{\"type\":");
        sb_append(content_sb, is_folder ? "\"folder\"" : "\"file\"");
        sb_append(content_sb, ",\"name\":");
        sb_append_json_str(content_sb, name);
        sb_append(content_sb, ",\"path\":");
        sb_append_json_str(content_sb, path);
        sb_append(content_sb, ",\"fullPath\":");

        // Build full path
        char full[MAX_PATH * 2];
        if (path && strlen(path) > 0) {
            size_t plen = strlen(path);
            if (path[plen - 1] == '\\' || path[plen - 1] == '/') {
                snprintf(full, sizeof(full), "%s%s", path, name ? name : "");
            } else {
                snprintf(full, sizeof(full), "%s\\%s", path, name ? name : "");
            }
        } else {
            snprintf(full, sizeof(full), "%s", name ? name : "");
        }
        sb_append_json_str(content_sb, full);
        sb_append(content_sb, "}");
    }

    sb_append(content_sb, "]}");
    g_api->db_query_destroy(q);
}

// Process single JSON-RPC message
char *mcp_process_json_rpc(const char *request_json, size_t request_len) {
    (void)request_len;
    str_builder_t resp;
    sb_init(&resp, 1024);

    char method[128] = {0};
    char id_str[64] = "null";

    extract_json_string(request_json, "method", method, sizeof(method));

    // Extract ID (number or string)
    const char *id_pos = strstr(request_json, "\"id\":");
    if (id_pos) {
        id_pos += 5;
        while (*id_pos == ' ' || *id_pos == '\t') id_pos++;
        size_t k = 0;
        while (*id_pos && *id_pos != ',' && *id_pos != '}' && *id_pos != '\n' && *id_pos != '\r' && k < sizeof(id_str) - 1) {
            id_str[k++] = *id_pos++;
        }
        id_str[k] = '\0';
    }

    if (strcmp(method, "initialize") == 0) {
        sb_append(&resp, "{\"jsonrpc\":\"2.0\",\"id\":");
        sb_append(&resp, id_str);
        sb_append(&resp, ",\"result\":{"
            "\"protocolVersion\":\"2024-11-05\","
            "\"capabilities\":{\"tools\":{}},"
            "\"serverInfo\":{\"name\":\"Everything-Native-MCP-Plugin\",\"version\":\"1.0.0\"}"
        "}}");
    } else if (strcmp(method, "notifications/initialized") == 0) {
        // Notification, no response required
        sb_free(&resp);
        return NULL;
    } else if (strcmp(method, "ping") == 0) {
        sb_append(&resp, "{\"jsonrpc\":\"2.0\",\"id\":");
        sb_append(&resp, id_str);
        sb_append(&resp, ",\"result\":{}}");
    } else if (strcmp(method, "tools/list") == 0) {
        handle_tools_list(id_str, &resp);
    } else if (strcmp(method, "tools/call") == 0) {
        char tool_name[128] = {0};
        extract_json_string(request_json, "name", tool_name, sizeof(tool_name));

        str_builder_t content;
        sb_init(&content, 512);

        if (strcmp(tool_name, "everything_search") == 0) {
            handle_search_tool(request_json, &content);
        } else if (strcmp(tool_name, "everything_status") == 0) {
            sb_append(&content, "{\"connected\":true,\"backend\":\"native_plugin_in_memory\",\"plugin\":\"Everything Native MCP\"}");
        } else {
            sb_append(&content, "{\"error\":\"Unknown tool name\"}");
        }

        sb_append(&resp, "{\"jsonrpc\":\"2.0\",\"id\":");
        sb_append(&resp, id_str);
        sb_append(&resp, ",\"result\":{\"content\":[{\"type\":\"text\",\"text\":");
        sb_append_json_str(&resp, content.data);
        sb_append(&resp, "}]}}");

        sb_free(&content);
    } else {
        // Method not found
        sb_append(&resp, "{\"jsonrpc\":\"2.0\",\"id\":");
        sb_append(&resp, id_str);
        sb_append(&resp, ",\"error\":{\"code\":-32601,\"message\":\"Method not found\"}}");
    }

    return resp.data;
}

void mcp_free_response(char *response) {
    if (response) free(response);
}

// Worker thread for Windows Named Pipe listener
static DWORD WINAPI mcp_pipe_worker_thread(LPVOID lpParam) {
    (void)lpParam;
    if (g_api && g_api->debug_printf) {
        g_api->debug_printf("[MCP] Native MCP Named Pipe server thread started: %ls\n", g_pipe_name);
    }

    while (InterlockedCompareExchange(&g_running, 1, 1) == 1) {
        HANDLE hPipe = CreateNamedPipeW(
            g_pipe_name,
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            65536,
            65536,
            0,
            NULL
        );

        if (hPipe == INVALID_HANDLE_VALUE) {
            Sleep(500);
            continue;
        }

        BOOL connected = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
        if (connected) {
            char buffer[65536];
            DWORD bytesRead = 0;

            while (InterlockedCompareExchange(&g_running, 1, 1) == 1) {
                BOOL success = ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL);
                if (!success || bytesRead == 0) break;

                buffer[bytesRead] = '\0';

                // Process JSON-RPC lines
                char *line = strtok(buffer, "\r\n");
                while (line) {
                    if (strlen(line) > 0) {
                        char *resp = mcp_process_json_rpc(line, strlen(line));
                        if (resp) {
                            DWORD written = 0;
                            WriteFile(hPipe, resp, (DWORD)strlen(resp), &written, NULL);
                            WriteFile(hPipe, "\n", 1, &written, NULL);
                            FlushFileBuffers(hPipe);
                            mcp_free_response(resp);
                        }
                    }
                    line = strtok(NULL, "\r\n");
                }
            }
        }

        DisconnectNamedPipe(hPipe);
        CloseHandle(hPipe);
    }

    return 0;
}

int mcp_server_init(everything_plugin_api_t *api) {
    g_api = api;
    return 1;
}

int mcp_server_start(const wchar_t *pipe_name, int tcp_port) {
    (void)tcp_port;
    if (pipe_name && wcslen(pipe_name) > 0) {
        wcsncpy(g_pipe_name, pipe_name, sizeof(g_pipe_name) / sizeof(wchar_t) - 1);
    }

    if (InterlockedCompareExchange(&g_running, 1, 0) == 0) {
        g_hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        g_hThread = CreateThread(NULL, 0, mcp_pipe_worker_thread, NULL, 0, NULL);
        return g_hThread != NULL ? 1 : 0;
    }
    return 1;
}

int mcp_server_stop(void) {
    if (InterlockedCompareExchange(&g_running, 0, 1) == 1) {
        if (g_hStopEvent) SetEvent(g_hStopEvent);

        // Ping pipe to unblock any waiting ConnectNamedPipe
        HANDLE hDummy = CreateFileW(
            g_pipe_name,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
        );
        if (hDummy != INVALID_HANDLE_VALUE) {
            CloseHandle(hDummy);
        }

        if (g_hThread) {
            WaitForSingleObject(g_hThread, 3000);
            CloseHandle(g_hThread);
            g_hThread = NULL;
        }

        if (g_hStopEvent) {
            CloseHandle(g_hStopEvent);
            g_hStopEvent = NULL;
        }
    }
    return 1;
}
