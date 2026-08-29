//
// Everything 1.5 Native MCP Protocol & Tool Execution Engine
//

#define _WIN32_WINNT 0x0600
#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "mcp_server.h"
#include "version.h"

// External host functions from plugin_main.c
extern everything_plugin_db_query_t* (*everything_plugin_db_query_create)(everything_plugin_db_t *db, void *event_proc, void *user_data);
extern void (*everything_plugin_db_query_destroy)(everything_plugin_db_query_t *q);
extern int (*everything_plugin_db_query_search)(everything_plugin_db_query_t *q, int match_case, int match_whole_word, int match_path, int match_diacritics, int match_prefix, int match_suffix, int ignore_punctuation, int ignore_whitespace, int match_regex, int hide_empty_search_results, int clear_selection, int clear_item_refs, const everything_plugin_utf8_t *search_string, int fast_sort_only, const everything_plugin_property_t *sort_property_type, int sort_ascending, const everything_plugin_property_t *sort_property_type2, int sort_ascending2, const everything_plugin_property_t *sort_property_type3, int sort_ascending3, int folders_first, int track_selected_and_total_file_size, int track_selected_folder_size, int force, int allow_query_access, int allow_read_access, int allow_disk_access, int hide_omit_results, int size_standard, int sort_mix);
extern uintptr_t (*everything_plugin_db_query_get_result_count)(const everything_plugin_db_query_t *q);
extern void (*everything_plugin_db_query_get_result_name)(everything_plugin_db_query_t *q, uintptr_t index, everything_plugin_utf8_buf_t *cbuf);
extern void (*everything_plugin_db_query_get_result_path)(everything_plugin_db_query_t *q, uintptr_t index, everything_plugin_utf8_buf_t *cbuf);
extern void (*everything_plugin_db_query_get_result_indexed_fd)(everything_plugin_db_query_t *q, uintptr_t index, everything_plugin_fileinfo_fd_t *fd);
extern int (*everything_plugin_db_query_is_folder_result)(everything_plugin_db_query_t *q, uintptr_t index);
extern void (*everything_plugin_utf8_buf_init)(everything_plugin_utf8_buf_t *cbuf);
extern void (*everything_plugin_utf8_buf_kill)(everything_plugin_utf8_buf_t *cbuf);

// Dynamic string buffer helper
typedef struct
{
	char *data;
	size_t len;
	size_t cap;
} str_buf_t;

static void str_buf_init(str_buf_t *sb)
{
	sb->len = 0;
	sb->cap = 4096;
	sb->data = (char *)malloc(sb->cap);
	if (sb->data)
	{
		sb->data[0] = '\0';
	}
	else
	{
		sb->cap = 0;
	}
}

static void str_buf_free(str_buf_t *sb)
{
	if (sb->data)
	{
		free(sb->data);
		sb->data = NULL;
	}
	sb->len = 0;
	sb->cap = 0;
}

static void str_buf_append(str_buf_t *sb, const char *str)
{
	if (!str || !sb->data) return;
	size_t slen = strlen(str);
	if (sb->len + slen + 1 > sb->cap)
	{
		size_t new_cap = (sb->cap * 2) > (sb->len + slen + 1) ? (sb->cap * 2) : (sb->len + slen + 4096);
		char *new_data = (char *)realloc(sb->data, new_cap);
		if (!new_data) return;
		sb->data = new_data;
		sb->cap = new_cap;
	}
	memcpy(sb->data + sb->len, str, slen);
	sb->len += slen;
	sb->data[sb->len] = '\0';
}

static void str_buf_append_json_str(str_buf_t *sb, const char *str)
{
	str_buf_append(sb, "\"");
	if (str)
	{
		for (const char *p = str; *p; p++)
		{
			switch (*p)
			{
				case '\"': str_buf_append(sb, "\\\""); break;
				case '\\': str_buf_append(sb, "\\\\"); break;
				case '\b': str_buf_append(sb, "\\b"); break;
				case '\f': str_buf_append(sb, "\\f"); break;
				case '\n': str_buf_append(sb, "\\n"); break;
				case '\r': str_buf_append(sb, "\\r"); break;
				case '\t': str_buf_append(sb, "\\t"); break;
				default:
					if ((unsigned char)*p < 0x20)
					{
						char ubuf[16];
						snprintf(ubuf, sizeof(ubuf), "\\u%04x", (unsigned char)*p);
						str_buf_append(sb, ubuf);
					}
					else
					{
						char cbuf[2] = { *p, '\0' };
						str_buf_append(sb, cbuf);
					}
					break;
			}
		}
	}
	str_buf_append(sb, "\"");
}

// Simple JSON field extractors
static bool json_get_string(const char *json, const char *key, char *out, size_t out_size)
{
	char needle[128];
	snprintf(needle, sizeof(needle), "\"%s\":", key);
	const char *pos = strstr(json, needle);
	if (!pos)
	{
		snprintf(needle, sizeof(needle), "\"%s\" :", key);
		pos = strstr(json, needle);
	}
	if (!pos) return false;

	pos = strchr(pos, ':');
	if (!pos) return false;
	pos++;
	while (*pos == ' ' || *pos == '\t' || *pos == '\r' || *pos == '\n') pos++;

	if (*pos == '\"')
	{
		pos++;
		size_t i = 0;
		while (*pos && *pos != '\"' && i + 1 < out_size)
		{
			if (*pos == '\\' && *(pos + 1))
			{
				pos++;
				if (*pos == 'n') out[i++] = '\n';
				else if (*pos == 'r') out[i++] = '\r';
				else if (*pos == 't') out[i++] = '\t';
				else if (*pos == '\"') out[i++] = '\"';
				else if (*pos == '\\') out[i++] = '\\';
				else out[i++] = *pos;
			}
			else
			{
				out[i++] = *pos;
			}
			pos++;
		}
		out[i] = '\0';
		return true;
	}
	return false;
}

static bool json_get_int(const char *json, const char *key, int *out)
{
	char needle[128];
	snprintf(needle, sizeof(needle), "\"%s\":", key);
	const char *pos = strstr(json, needle);
	if (!pos)
	{
		snprintf(needle, sizeof(needle), "\"%s\" :", key);
		pos = strstr(json, needle);
	}
	if (!pos) return false;

	pos = strchr(pos, ':');
	if (!pos) return false;
	pos++;
	while (*pos == ' ' || *pos == '\t' || *pos == '\r' || *pos == '\n') pos++;

	*out = atoi(pos);
	return true;
}

static uint64_t filetime_to_unix_ms(uint64_t ft)
{
	if (ft == 0) return 0;
	// 116444736000000000 is 1601 to 1970 in 100ns units
	if (ft < 116444736000000000ULL) return 0;
	return (ft - 116444736000000000ULL) / 10000ULL;
}

// Server Worker Threads
static HANDLE g_pipe_thread = NULL;
static HANDLE g_http_thread = NULL;
static volatile bool g_server_running = false;
static SOCKET g_http_listen_socket = INVALID_SOCKET;

// Search execution in memory
char *mcp_execute_search(const char *query, int offset, int max_count, int sort_type, int ascending)
{
	(void)sort_type;
	(void)ascending;

	str_buf_t sb;
	str_buf_init(&sb);

	if (!g_everything_db || !everything_plugin_db_query_create || !everything_plugin_db_query_search)
	{
		str_buf_append(&sb, "{\"error\": \"Everything database query API is not available\"}");
		return sb.data;
	}

	if (offset < 0) offset = 0;
	if (max_count <= 0) max_count = g_mcp_max_results > 0 ? g_mcp_max_results : 100;
	if (max_count > 500) max_count = 500;

	everything_plugin_db_query_t *db_query = everything_plugin_db_query_create(g_everything_db, NULL, NULL);
	if (!db_query)
	{
		str_buf_append(&sb, "{\"error\": \"Failed to create database query\"}");
		return sb.data;
	}

	everything_plugin_db_query_search(db_query, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, (const everything_plugin_utf8_t *)query, 0, NULL, 1, NULL, 0, NULL, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0);

	uintptr_t total_results = everything_plugin_db_query_get_result_count ? everything_plugin_db_query_get_result_count(db_query) : 0;

	str_buf_append(&sb, "{\n  \"total\": ");
	char num_buf[32];
	snprintf(num_buf, sizeof(num_buf), "%zu", total_results);
	str_buf_append(&sb, num_buf);
	str_buf_append(&sb, ",\n  \"offset\": ");
	snprintf(num_buf, sizeof(num_buf), "%d", offset);
	str_buf_append(&sb, num_buf);
	str_buf_append(&sb, ",\n  \"count\": ");

	uintptr_t end = (uintptr_t)offset + (uintptr_t)max_count;
	if (end > total_results) end = total_results;
	uintptr_t returned = (end > (uintptr_t)offset) ? (end - (uintptr_t)offset) : 0;

	snprintf(num_buf, sizeof(num_buf), "%zu", returned);
	str_buf_append(&sb, num_buf);
	str_buf_append(&sb, ",\n  \"results\": [\n");

	everything_plugin_utf8_buf_t name_buf;
	everything_plugin_utf8_buf_t path_buf;
	if (everything_plugin_utf8_buf_init)
	{
		everything_plugin_utf8_buf_init(&name_buf);
		everything_plugin_utf8_buf_init(&path_buf);
	}

	for (uintptr_t i = (uintptr_t)offset; i < end; i++)
	{
		if (i > (uintptr_t)offset) str_buf_append(&sb, ",\n");

		const char *name_str = "";
		const char *path_str = "";

		if (everything_plugin_db_query_get_result_name)
		{
			everything_plugin_db_query_get_result_name(db_query, i, &name_buf);
			name_str = (const char *)name_buf.buf;
		}
		if (everything_plugin_db_query_get_result_path)
		{
			everything_plugin_db_query_get_result_path(db_query, i, &path_buf);
			path_str = (const char *)path_buf.buf;
		}

		int is_folder = everything_plugin_db_query_is_folder_result ? everything_plugin_db_query_is_folder_result(db_query, i) : 0;
		everything_plugin_fileinfo_fd_t fd = {0};
		if (everything_plugin_db_query_get_result_indexed_fd)
		{
			everything_plugin_db_query_get_result_indexed_fd(db_query, i, &fd);
		}

		char full_path[1024];
		if (path_str && *path_str)
		{
			size_t plen = strlen(path_str);
			if (path_str[plen - 1] == '\\') snprintf(full_path, sizeof(full_path), "%s%s", path_str, name_str);
			else snprintf(full_path, sizeof(full_path), "%s\\%s", path_str, name_str);
		}
		else
		{
			snprintf(full_path, sizeof(full_path), "%s", name_str);
		}

		str_buf_append(&sb, "    {\n      \"name\": ");
		str_buf_append_json_str(&sb, name_str);
		str_buf_append(&sb, ",\n      \"path\": ");
		str_buf_append_json_str(&sb, path_str);
		str_buf_append(&sb, ",\n      \"full_path\": ");
		str_buf_append_json_str(&sb, full_path);
		str_buf_append(&sb, ",\n      \"type\": ");
		str_buf_append_json_str(&sb, is_folder ? "folder" : "file");
		str_buf_append(&sb, ",\n      \"size\": ");
		snprintf(num_buf, sizeof(num_buf), "%llu", (unsigned long long)fd.size);
		str_buf_append(&sb, num_buf);
		str_buf_append(&sb, ",\n      \"date_modified_ms\": ");
		snprintf(num_buf, sizeof(num_buf), "%llu", (unsigned long long)filetime_to_unix_ms(fd.date_modified));
		str_buf_append(&sb, num_buf);
		str_buf_append(&sb, "\n    }");
	}

	if (everything_plugin_utf8_buf_kill)
	{
		everything_plugin_utf8_buf_kill(&name_buf);
		everything_plugin_utf8_buf_kill(&path_buf);
	}

	str_buf_append(&sb, "\n  ]\n}");
	if (everything_plugin_db_query_destroy) everything_plugin_db_query_destroy(db_query);

	return sb.data;
}

char *mcp_execute_file_info(const char *path)
{
	str_buf_t sb;
	str_buf_init(&sb);

	WIN32_FILE_ATTRIBUTE_DATA attr;
	if (GetFileAttributesExA(path, GetFileExInfoStandard, &attr))
	{
		ULARGE_INTEGER size;
		size.LowPart = attr.nFileSizeLow;
		size.HighPart = attr.nFileSizeHigh;

		bool is_dir = (attr.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

		str_buf_append(&sb, "{\n  \"path\": ");
		str_buf_append_json_str(&sb, path);
		str_buf_append(&sb, ",\n  \"exists\": true");
		str_buf_append(&sb, ",\n  \"type\": ");
		str_buf_append_json_str(&sb, is_dir ? "folder" : "file");
		str_buf_append(&sb, ",\n  \"size\": ");
		char num_buf[32];
		snprintf(num_buf, sizeof(num_buf), "%llu", (unsigned long long)size.QuadPart);
		str_buf_append(&sb, num_buf);
		str_buf_append(&sb, ",\n  \"date_modified_ms\": ");
		ULARGE_INTEGER dm;
		dm.LowPart = attr.ftLastWriteTime.dwLowDateTime;
		dm.HighPart = attr.ftLastWriteTime.dwHighDateTime;
		snprintf(num_buf, sizeof(num_buf), "%llu", (unsigned long long)filetime_to_unix_ms(dm.QuadPart));
		str_buf_append(&sb, num_buf);
		str_buf_append(&sb, "\n}");
	}
	else
	{
		str_buf_append(&sb, "{\n  \"path\": ");
		str_buf_append_json_str(&sb, path);
		str_buf_append(&sb, ",\n  \"exists\": false,\n  \"error\": \"File not found or access denied\"\n}");
	}

	return sb.data;
}

char *mcp_execute_preview(const char *path, int max_lines, int max_bytes)
{
	str_buf_t sb;
	str_buf_init(&sb);

	if (!g_mcp_allow_preview)
	{
		str_buf_append(&sb, "{\"error\": \"File preview is disabled in Everything MCP plugin options.\"}");
		return sb.data;
	}

	if (max_lines <= 0) max_lines = 50;
	if (max_bytes <= 0) max_bytes = 16384;
	if (max_bytes > 131072) max_bytes = 131072;

	FILE *f = fopen(path, "rb");
	if (!f)
	{
		str_buf_append(&sb, "{\"error\": \"Unable to open file for reading\"}");
		return sb.data;
	}

	char *buf = (char *)malloc(max_bytes + 1);
	if (!buf)
	{
		fclose(f);
		str_buf_append(&sb, "{\"error\": \"Out of memory\"}");
		return sb.data;
	}

	size_t read_bytes = fread(buf, 1, max_bytes, f);
	buf[read_bytes] = '\0';
	fclose(f);

	// Check for null bytes (binary file test)
	bool is_binary = false;
	for (size_t i = 0; i < read_bytes; i++)
	{
		if (buf[i] == '\0') { is_binary = true; break; }
	}

	if (is_binary)
	{
		free(buf);
		str_buf_append(&sb, "{\"error\": \"Binary file detected. Preview only supports text files.\"}");
		return sb.data;
	}

	// Truncate to max_lines
	int lines = 0;
	size_t cut_pos = read_bytes;
	for (size_t i = 0; i < read_bytes; i++)
	{
		if (buf[i] == '\n')
		{
			lines++;
			if (lines >= max_lines)
			{
				cut_pos = i + 1;
				break;
			}
		}
	}
	buf[cut_pos] = '\0';

	str_buf_append(&sb, "{\n  \"path\": ");
	str_buf_append_json_str(&sb, path);
	str_buf_append(&sb, ",\n  \"lines\": ");
	char num_buf[32];
	snprintf(num_buf, sizeof(num_buf), "%d", lines);
	str_buf_append(&sb, num_buf);
	str_buf_append(&sb, ",\n  \"content\": ");
	str_buf_append_json_str(&sb, buf);
	str_buf_append(&sb, "\n}");

	free(buf);
	return sb.data;
}

char *mcp_execute_status(void)
{
	str_buf_t sb;
	str_buf_init(&sb);

	str_buf_append(&sb, "{\n  \"status\": \"online\",\n  \"plugin\": \"Everything 1.5 Native MCP Server\",\n  \"version\": \"" PLUGINVERSION "\",\n  \"transport\": [\"named_pipe\", \"http_sse\"],\n  \"pipe_name\": ");
	str_buf_append_json_str(&sb, g_mcp_pipe_name);
	str_buf_append(&sb, ",\n  \"http_port\": ");
	char num_buf[32];
	snprintf(num_buf, sizeof(num_buf), "%d", g_mcp_http_port);
	str_buf_append(&sb, num_buf);
	str_buf_append(&sb, ",\n  \"allow_preview\": ");
	str_buf_append(&sb, g_mcp_allow_preview ? "true" : "false");
	str_buf_append(&sb, "\n}");

	return sb.data;
}

// JSON-RPC Dispatcher
char *mcp_handle_jsonrpc(const char *req)
{
	str_buf_t sb;
	str_buf_init(&sb);

	char id_str[64] = "1";
	json_get_string(req, "id", id_str, sizeof(id_str));
	if (strcmp(id_str, "1") == 0)
	{
		int id_num = 1;
		if (json_get_int(req, "id", &id_num))
		{
			snprintf(id_str, sizeof(id_str), "%d", id_num);
		}
	}

	char method[128] = "";
	json_get_string(req, "method", method, sizeof(method));

	if (strcmp(method, "initialize") == 0)
	{
		str_buf_append(&sb, "{\"jsonrpc\":\"2.0\",\"id\":");
		if (id_str[0] >= '0' && id_str[0] <= '9') str_buf_append(&sb, id_str);
		else str_buf_append_json_str(&sb, id_str);
		str_buf_append(&sb, ",\"result\":{\"protocolVersion\":\"2024-11-05\",\"serverInfo\":{\"name\":\"everything-native-plugin\",\"version\":\"" PLUGINVERSION "\"},\"capabilities\":{\"tools\":{}}}}");
		return sb.data;
	}
	else if (strcmp(method, "notifications/initialized") == 0 || strcmp(method, "initialized") == 0)
	{
		str_buf_append(&sb, "");
		return sb.data;
	}
	else if (strcmp(method, "ping") == 0)
	{
		str_buf_append(&sb, "{\"jsonrpc\":\"2.0\",\"id\":");
		if (id_str[0] >= '0' && id_str[0] <= '9') str_buf_append(&sb, id_str);
		else str_buf_append_json_str(&sb, id_str);
		str_buf_append(&sb, ",\"result\":{}}");
		return sb.data;
	}
	else if (strcmp(method, "tools/list") == 0)
	{
		str_buf_append(&sb, "{\"jsonrpc\":\"2.0\",\"id\":");
		if (id_str[0] >= '0' && id_str[0] <= '9') str_buf_append(&sb, id_str);
		else str_buf_append_json_str(&sb, id_str);
		str_buf_append(&sb, ",\"result\":{\"tools\":[");

		// Tool 1: everything_search
		str_buf_append(&sb, "{\"name\":\"everything_search\",\"description\":\"Execute an ultra-fast search query across the Everything 1.5 database with full syntax support (wildcards, regex, ext:, size:, dm:, etc.).\",\"inputSchema\":{\"type\":\"object\",\"properties\":{\"query\":{\"type\":\"string\",\"description\":\"Everything search syntax string\"},\"offset\":{\"type\":\"number\",\"description\":\"0-based index offset\"},\"count\":{\"type\":\"number\",\"description\":\"Max results to return (default 100)\"}},\"required\":[\"query\"]}},");

		// Tool 2: everything_find_files
		str_buf_append(&sb, "{\"name\":\"everything_find_files\",\"description\":\"Search specifically for files with structured filtering (extension, path, size, date).\",\"inputSchema\":{\"type\":\"object\",\"properties\":{\"name\":{\"type\":\"string\",\"description\":\"File name pattern\"},\"extension\":{\"type\":\"string\",\"description\":\"File extension (e.g. pdf, txt)\"},\"path\":{\"type\":\"string\",\"description\":\"Directory path to restrict to\"},\"count\":{\"type\":\"number\"},\"offset\":{\"type\":\"number\"}}}},");

		// Tool 3: everything_find_folders
		str_buf_append(&sb, "{\"name\":\"everything_find_folders\",\"description\":\"Search specifically for folders/directories across indexed drives.\",\"inputSchema\":{\"type\":\"object\",\"properties\":{\"name\":{\"type\":\"string\",\"description\":\"Folder name pattern\"},\"path\":{\"type\":\"string\",\"description\":\"Parent directory\"},\"count\":{\"type\":\"number\"},\"offset\":{\"type\":\"number\"}}}},");

		// Tool 4: everything_get_file_info
		str_buf_append(&sb, "{\"name\":\"everything_get_file_info\",\"description\":\"Retrieve full indexing metadata and file system attributes for a specific file or folder.\",\"inputSchema\":{\"type\":\"object\",\"properties\":{\"path\":{\"type\":\"string\",\"description\":\"Absolute path to the file/folder\"}},\"required\":[\"path\"]}},");

		// Tool 5: everything_preview_file
		str_buf_append(&sb, "{\"name\":\"everything_preview_file\",\"description\":\"Safely read and preview lines of a text file found in Everything search.\",\"inputSchema\":{\"type\":\"object\",\"properties\":{\"path\":{\"type\":\"string\",\"description\":\"Absolute path to text file\"},\"max_lines\":{\"type\":\"number\",\"description\":\"Max lines to return (default 50)\"},\"max_bytes\":{\"type\":\"number\",\"description\":\"Max bytes to read (default 16384)\"}},\"required\":[\"path\"]}},");

		// Tool 6: everything_status
		str_buf_append(&sb, "{\"name\":\"everything_status\",\"description\":\"Check native Everything MCP plugin status and server options.\",\"inputSchema\":{\"type\":\"object\",\"properties\":{}}}");

		str_buf_append(&sb, "]}}");
		return sb.data;
	}
	else if (strcmp(method, "tools/call") == 0)
	{
		char tool_name[128] = "";
		json_get_string(req, "name", tool_name, sizeof(tool_name));

		char *result_text = NULL;

		if (strcmp(tool_name, "everything_search") == 0)
		{
			char query[512] = "";
			int offset = 0;
			int count = g_mcp_max_results;
			json_get_string(req, "query", query, sizeof(query));
			json_get_int(req, "offset", &offset);
			json_get_int(req, "count", &count);
			result_text = mcp_execute_search(query, offset, count, 0, 1);
		}
		else if (strcmp(tool_name, "everything_find_files") == 0)
		{
			char name[256] = "";
			char ext[64] = "";
			char path[512] = "";
			int count = g_mcp_max_results;
			int offset = 0;
			json_get_string(req, "name", name, sizeof(name));
			json_get_string(req, "extension", ext, sizeof(ext));
			json_get_string(req, "path", path, sizeof(path));
			json_get_int(req, "count", &count);
			json_get_int(req, "offset", &offset);

			char qbuf[1024];
			snprintf(qbuf, sizeof(qbuf), "file: %s %s%s %s%s",
				name,
				ext[0] ? "ext:" : "", ext,
				path[0] ? "path:" : "", path);
			result_text = mcp_execute_search(qbuf, offset, count, 0, 1);
		}
		else if (strcmp(tool_name, "everything_find_folders") == 0)
		{
			char name[256] = "";
			char path[512] = "";
			int count = g_mcp_max_results;
			int offset = 0;
			json_get_string(req, "name", name, sizeof(name));
			json_get_string(req, "path", path, sizeof(path));
			json_get_int(req, "count", &count);
			json_get_int(req, "offset", &offset);

			char qbuf[1024];
			snprintf(qbuf, sizeof(qbuf), "folder: %s %s%s",
				name,
				path[0] ? "path:" : "", path);
			result_text = mcp_execute_search(qbuf, offset, count, 0, 1);
		}
		else if (strcmp(tool_name, "everything_get_file_info") == 0)
		{
			char path[512] = "";
			json_get_string(req, "path", path, sizeof(path));
			result_text = mcp_execute_file_info(path);
		}
		else if (strcmp(tool_name, "everything_preview_file") == 0)
		{
			char path[512] = "";
			int max_lines = 50;
			int max_bytes = 16384;
			json_get_string(req, "path", path, sizeof(path));
			json_get_int(req, "max_lines", &max_lines);
			json_get_int(req, "max_bytes", &max_bytes);
			result_text = mcp_execute_preview(path, max_lines, max_bytes);
		}
		else if (strcmp(tool_name, "everything_status") == 0)
		{
			result_text = mcp_execute_status();
		}
		else
		{
			str_buf_append(&sb, "{\"jsonrpc\":\"2.0\",\"id\":");
			if (id_str[0] >= '0' && id_str[0] <= '9') str_buf_append(&sb, id_str);
			else str_buf_append_json_str(&sb, id_str);
			str_buf_append(&sb, ",\"error\":{\"code\":-32601,\"message\":\"Method not found\"}}");
			return sb.data;
		}

		str_buf_append(&sb, "{\"jsonrpc\":\"2.0\",\"id\":");
		if (id_str[0] >= '0' && id_str[0] <= '9') str_buf_append(&sb, id_str);
		else str_buf_append_json_str(&sb, id_str);
		str_buf_append(&sb, ",\"result\":{\"content\":[{\"type\":\"text\",\"text\":");
		str_buf_append_json_str(&sb, result_text ? result_text : "");
		str_buf_append(&sb, "}]}}");

		if (result_text) free(result_text);
		return sb.data;
	}

	str_buf_append(&sb, "{\"jsonrpc\":\"2.0\",\"id\":");
	if (id_str[0] >= '0' && id_str[0] <= '9') str_buf_append(&sb, id_str);
	else str_buf_append_json_str(&sb, id_str);
	str_buf_append(&sb, ",\"error\":{\"code\":-32601,\"message\":\"Unknown method\"}}");
	return sb.data;
}

// Named Pipe Server Thread
static DWORD WINAPI mcp_pipe_server_thread(LPVOID param)
{
	(void)param;

	while (g_server_running)
	{
		HANDLE pipe = CreateNamedPipeA(
			g_mcp_pipe_name,
			PIPE_ACCESS_DUPLEX,
			PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
			PIPE_UNLIMITED_INSTANCES,
			65536,
			65536,
			0,
			NULL
		);

		if (pipe == INVALID_HANDLE_VALUE)
		{
			Sleep(500);
			continue;
		}

		BOOL connected = ConnectNamedPipe(pipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
		if (connected && g_server_running)
		{
			char read_buf[65536];
			str_buf_t acc;
			str_buf_init(&acc);

			DWORD bytes_read = 0;
			while (g_server_running && ReadFile(pipe, read_buf, sizeof(read_buf) - 1, &bytes_read, NULL) && bytes_read > 0)
			{
				read_buf[bytes_read] = '\0';
				str_buf_append(&acc, read_buf);

				// Process complete lines
				char *newline;
				while ((newline = strchr(acc.data, '\n')) != NULL)
				{
					*newline = '\0';
					char *line = acc.data;
					size_t line_len = strlen(line);

					if (line_len > 0)
					{
						if (line[line_len - 1] == '\r') line[line_len - 1] = '\0';
						if (strlen(line) > 0)
						{
							char *resp = mcp_handle_jsonrpc(line);
							if (resp && *resp)
							{
								DWORD written = 0;
								WriteFile(pipe, resp, (DWORD)strlen(resp), &written, NULL);
								WriteFile(pipe, "\n", 1, &written, NULL);
								FlushFileBuffers(pipe);
							}
							if (resp) free(resp);
						}
					}

					// Shift remaining
					size_t consumed = (newline - acc.data) + 1;
					size_t remaining = acc.len - consumed;
					memmove(acc.data, newline + 1, remaining);
					acc.len = remaining;
					acc.data[acc.len] = '\0';
				}
			}

			str_buf_free(&acc);
		}

		DisconnectNamedPipe(pipe);
		CloseHandle(pipe);
	}

	return 0;
}

// HTTP / SSE Server Thread
static DWORD WINAPI mcp_http_server_thread(LPVOID param)
{
	(void)param;

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

	g_http_listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (g_http_listen_socket == INVALID_SOCKET)
	{
		WSACleanup();
		return 1;
	}

	int opt = 1;
	setsockopt(g_http_listen_socket, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));

	struct sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	sin.sin_port = htons((unsigned short)g_mcp_http_port);

	if (bind(g_http_listen_socket, (struct sockaddr *)&sin, sizeof(sin)) == SOCKET_ERROR ||
		listen(g_http_listen_socket, SOMAXCONN) == SOCKET_ERROR)
	{
		closesocket(g_http_listen_socket);
		g_http_listen_socket = INVALID_SOCKET;
		WSACleanup();
		return 1;
	}

	while (g_server_running)
	{
		SOCKET client = accept(g_http_listen_socket, NULL, NULL);
		if (client == INVALID_SOCKET) break;

		char req_buf[65536];
		int n = recv(client, req_buf, sizeof(req_buf) - 1, 0);
		if (n > 0)
		{
			req_buf[n] = '\0';

			if (strstr(req_buf, "POST /mcp") || strstr(req_buf, "POST / "))
			{
				const char *body = strstr(req_buf, "\r\n\r\n");
				if (body)
				{
					body += 4;
					char *resp = mcp_handle_jsonrpc(body);
					if (resp)
					{
						char header[512];
						snprintf(header, sizeof(header),
							"HTTP/1.1 200 OK\r\n"
							"Content-Type: application/json; charset=utf-8\r\n"
							"Content-Length: %zu\r\n"
							"Access-Control-Allow-Origin: *\r\n"
							"Connection: close\r\n\r\n",
							strlen(resp));
						send(client, header, (int)strlen(header), 0);
						send(client, resp, (int)strlen(resp), 0);
						free(resp);
					}
				}
			}
			else
			{
				const char *status_resp = "{\"status\":\"Everything 1.5 MCP Native Plugin Online\"}";
				char header[512];
				snprintf(header, sizeof(header),
					"HTTP/1.1 200 OK\r\n"
					"Content-Type: application/json; charset=utf-8\r\n"
					"Content-Length: %zu\r\n"
					"Access-Control-Allow-Origin: *\r\n"
					"Connection: close\r\n\r\n",
					strlen(status_resp));
				send(client, header, (int)strlen(header), 0);
				send(client, status_resp, (int)strlen(status_resp), 0);
			}
		}

		closesocket(client);
	}

	if (g_http_listen_socket != INVALID_SOCKET)
	{
		closesocket(g_http_listen_socket);
		g_http_listen_socket = INVALID_SOCKET;
	}
	WSACleanup();
	return 0;
}

int mcp_server_init(void *db)
{
	g_everything_db = db;
	return 1;
}

void mcp_server_apply_settings(void)
{
	mcp_server_shutdown();

	if (g_mcp_enabled)
	{
		g_server_running = true;
		g_pipe_thread = CreateThread(NULL, 0, mcp_pipe_server_thread, NULL, 0, NULL);
		g_http_thread = CreateThread(NULL, 0, mcp_http_server_thread, NULL, 0, NULL);
	}
}

void mcp_server_shutdown(void)
{
	if (g_server_running)
	{
		g_server_running = false;

		if (g_http_listen_socket != INVALID_SOCKET)
		{
			closesocket(g_http_listen_socket);
			g_http_listen_socket = INVALID_SOCKET;
		}

		// Wake up pipe server thread
		HANDLE pipe = CreateFileA(g_mcp_pipe_name, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (pipe != INVALID_HANDLE_VALUE) CloseHandle(pipe);

		if (g_pipe_thread)
		{
			WaitForSingleObject(g_pipe_thread, 1000);
			CloseHandle(g_pipe_thread);
			g_pipe_thread = NULL;
		}

		if (g_http_thread)
		{
			WaitForSingleObject(g_http_thread, 1000);
			CloseHandle(g_http_thread);
			g_http_thread = NULL;
		}
	}
}

void mcp_server_destroy(void)
{
	mcp_server_shutdown();
}
