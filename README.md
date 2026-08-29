# Everything MCP Server

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

一个对接 [voidtools Everything](https://www.voidtools.com/) 的 Model Context Protocol (MCP) 服务。通过此服务，AI 助手（如 Claude Desktop、Cursor、Cline 等）可以直接调用本地 Everything 实例进行文件检索、属性查询与文本内容预览。

---

## 功能说明

- **搜索集成**：支持 Everything 搜索语法（通配符 `*` `?`、布尔组合、`ext:`、`size:`、`dm:`、`regex:` 等）。
- **工具集**：
  - `everything_search`：通用搜索，支持完整语法、排序与分页。
  - `everything_find_files`：按文件名、扩展名、路径、大小和修改时间筛选文件。
  - `everything_find_folders`：按名称和父目录筛选文件夹。
  - `everything_get_file_info`：获取指定路径在 Everything 数据库中的索引信息与本地文件系统属性。
  - `everything_preview_file`：读取指定文本文件的部分内容（带行号，包含二进制文件检测）。
  - `everything_status`：检查 Everything 服务连接状态与已索引项目总数。
- **数据格式化**：自动将 Windows FILETIME 时间戳转换为 ISO 8601 格式，将字节大小转换为常见单位（B/KB/MB/GB）。
- **连接方式**：通过 Everything HTTP Server 插件通信，默认自动检测 `http://127.0.0.1:8088` 及本地 `Plugins.ini` 配置。

---

## 前置要求

1. 安装并运行 **Everything**（推荐 1.5 版本，亦兼容 1.4）。
2. 启用 **HTTP 服务器**：
   - 打开 Everything -> `工具 (Tools)` -> `选项 (Options)`。
   - 在左侧选择 `插件 (Plug-ins)` -> `HTTP 服务器`（或 `http_server64.dll`）。
   - 勾选 `启用 HTTP 服务器`，端口保持默认 `8088`（或自定义），点击确定。
3. 安装 **Node.js**（>= 18.0.0）。

---

## 客户端配置

### 1. Claude Desktop (`claude_desktop_config.json`)

```json
{
  "mcpServers": {
    "everything": {
      "command": "node",
      "args": [
        "D:\\Set\\Everything\\EverythingPlugin\\Everything MCP\\dist\\index.js"
      ],
      "env": {
        "EVERYTHING_HTTP_URL": "http://127.0.0.1:8088"
      }
    }
  }
}
```

### 2. Cursor / Cline / Antigravity

```json
{
  "mcpServers": {
    "everything": {
      "command": "node",
      "args": [
        "D:/Set/Everything/EverythingPlugin/Everything MCP/dist/index.js"
      ]
    }
  }
}
```

---

## 环境变量配置

若需自定义配置，可在项目根目录创建 `.env` 文件（或参考 `.env.example`）：

| 变量名 | 说明 | 默认值 |
| :--- | :--- | :--- |
| `EVERYTHING_HTTP_URL` | Everything HTTP 服务地址 | `http://127.0.0.1:8088` |
| `EVERYTHING_HTTP_USERNAME` | HTTP 认证用户名（如启用） | 空 |
| `EVERYTHING_HTTP_PASSWORD` | HTTP 认证密码（如启用） | 空 |

---

## 工具列表与参数

### 1. `everything_search`
执行 Everything 查询。
- `query` (string, 必填): 搜索语法字符串，例如 `*.pdf size:>10mb dm:today`。
- `max_results` (number, 可选, 默认 30): 返回的最大结果数量（上限 500）。
- `offset` (number, 可选, 默认 0): 分页偏移量。
- `sort` (enum, 可选): 排序字段（`name`, `path`, `size`, `extension`, `date_modified`, `date_created`, `attributes`）。
- `ascending` (boolean, 可选, 默认 true): 是否升序。
- `match_case` (boolean, 可选, 默认 false): 是否区分大小写。
- `match_whole_word` (boolean, 可选, 默认 false): 是否全字匹配。
- `match_path` (boolean, 可选, 默认 false): 是否匹配完整路径。
- `regex` (boolean, 可选, 默认 false): 是否启用正则表达式。
- `type_filter` (enum, 可选, 默认 `all`): 过滤类型（`all` / `files` / `folders`）。

### 2. `everything_find_files`
结构化查找文件。
- `name` (string, 可选): 文件名关键字或模式。
- `extension` (string, 可选): 文件扩展名，多个可用分号分隔（如 `ts;js;json`）。
- `directory` (string, 可选): 限定搜索目录。
- `min_size` / `max_size` (string, 可选): 大小限制（如 `1MB`, `500KB`）。
- `modified_after` / `modified_before` (string, 可选): 修改日期限制（如 `today`, `2026-01-01`）。
- `max_results` (number, 可选, 默认 30): 返回数量。
- `offset` (number, 可选, 默认 0): 偏移量。

### 3. `everything_find_folders`
结构化查找文件夹。
- `name` (string, 可选): 文件夹名称关键字。
- `parent_directory` (string, 可选): 父目录路径。
- `max_results` (number, 可选, 默认 30): 返回数量。
- `offset` (number, 可选, 默认 0): 偏移量。

### 4. `everything_get_file_info`
获取指定路径的索引与文件系统信息。
- `path` (string, 必填): 目标文件的完整绝对路径。

### 5. `everything_preview_file`
读取文本文件的部分行。
- `path` (string, 必填): 目标文件路径。
- `start_line` (number, 可选, 默认 1): 起始行号（从 1 开始）。
- `max_lines` (number, 可选, 默认 50): 最大读取行数（上限 200）。

### 6. `everything_status`
检查 Everything 连接状态、响应延迟与索引总数。

---

## 本地开发与构建

```bash
# 安装依赖
npm install

# 运行测试
npm test

# 编译 TypeScript 至 dist/
npm run build

# 以开发模式运行
npm run dev
```

---

## 文档存档

- [Everything 1.5 插件系统与 SDK 中文开发文档存档](docs/Everything_1.5_Plugins_and_SDK_CN.md)

---

## 版权与声明

- **Everything** 与 **Everything 搜索引擎** 归 **David Carpenter (voidtools)** 所有。
- 本项目为独立的社区 MCP 开源实现，与 voidtools 无官方关联。
- 许可协议：[MIT License](LICENSE)。
