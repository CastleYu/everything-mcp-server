# Everything MCP Server

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![MCP Version](https://img.shields.io/badge/MCP-1.6.0-green.svg)](https://modelcontextprotocol.io)

> 基于 **voidtools Everything 1.5 / 1.4** 的高性能 Model Context Protocol (MCP) 服务器，为 AI 助手（Claude Desktop、Cursor、Cline、Antigravity、Gemini CLI 等）提供毫秒级全盘文件搜索、路径定位、属性检索与文本预览能力。

---

## ⚖️ 商标与版权声明 (Legal & Trademark Notices)

- **Everything™** 及 **Everything 搜索引擎** 版权归 **David Carpenter (voidtools)** 所有。
- 本项目是面向 AI 与 MCP 生态的独立开源客户端实现，**非 voidtools 官方发布产品**，与 voidtools 无官方附属或赞助关系。
- 官方网站：[https://www.voidtools.com](https://www.voidtools.com)
- 官方技术论坛：[https://www.voidtools.com/forum/](https://www.voidtools.com/forum/)

---

## 📚 项目文档与背景

- 📖 **[Everything 1.5 插件系统与 SDK 中文开发文档存档](docs/Everything_1.5_Plugins_and_SDK_CN.md)**
  （基于官方论坛专帖 [Everything 1.5 Plug-ins](https://www.voidtools.com/forum/viewtopic.php?f=12&t=9799) 及 [Plugin SDK #16535](https://www.voidtools.com/forum/viewtopic.php?t=16535) 翻译整理）

---

## ✨ 核心特性

- ⚡ **毫秒级极速检索**：借助 Everything 的 NTFS/ReFS/USN Journal 内存级索引，千万级文件瞬间返回。
- 🔍 **全功能语法支持**：支持 Everything 强大的搜索语法（通配符 `*` `?`、布尔运算 `AND` `OR` `NOT` `|`、大小过滤 `size:>500MB`、日期过滤 `dm:today` `dc:thisweek`、正则 `regex:`、扩展名过滤 `ext:ts;js;json` 等）。
- 🛠️ **6 款实用 MCP 工具**：
  1. `everything_search`: 全功能通用搜索（支持高级语法、排序、分页与丰富过滤选项）。
  2. `everything_find_files`: 结构化文件查找辅助工具（按名称、扩展名、目录、大小与日期范围快速筛选）。
  3. `everything_find_folders`: 目录/文件夹查找工具。
  4. `everything_get_file_info`: 文件/文件夹元数据检索（结合 Everything 索引信息与本地文件系统信息）。
  5. `everything_preview_file`: 安全文本预览工具（附带行号显示与二进制文件自动识别检测）。
  6. `everything_status`: Everything 实例与 HTTP Server 连通性、索引统计与健康探测。
- 🔄 **智能自动发现**：自动检测运行中的 Everything 实例，自动读取 `Plugins.ini` 端口配置（默认探测 8088、80 端口），支持 Basic Auth 认证。
- 📦 **标准化时间与格式化**：自动将 Windows FILETIME 64位纳秒时间戳转化为 ISO 8601 标准时间，自动将字节数转化为易读大小（如 `14.65 KB`, `1.2 GB`）。

---

## 🚀 快速接入指南

### 1. 前置条件：开启 Everything HTTP 服务器插件

由于 Everything 1.5 已将 HTTP Server 模块解耦为独立插件，请确保 HTTP 服务器已启用：
1. 打开 **Everything** 主界面。
2. 点击菜单 `工具 (Tools) -> 选项 (Options)`。
3. 点击左侧 `插件 (Plug-ins) -> HTTP 服务器 (HTTP Server)`（或 `http_server64.dll`）。
4. 勾选 **启用 HTTP 服务器 (Enable HTTP Server)**。
5. 端口保持默认 `8088`（或按需自定义）。
6. 点击 **确定** 即可。

---

### 2. 客户端配置

#### Claude Desktop 配置 (`claude_desktop_config.json`)
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

#### Cursor / Cline / Antigravity MCP 配置
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

## 🔧 MCP 工具清单

### 1. `everything_search`
通用高阶搜索工具。
- **参数**：
  - `query` (string, 必填): Everything 搜索语法字符串（如 `*.config.json size:>10kb`）
  - `max_results` (number, 可选, 默认 30, 最大 500)
  - `offset` (number, 可选, 默认 0)
  - `sort` (enum, 可选): `name` | `path` | `size` | `extension` | `date_modified` | `date_created` | `attributes`
  - `ascending` (boolean, 可选, 默认 true)
  - `match_case` (boolean, 可选, 默认 false)
  - `match_whole_word` (boolean, 可选, 默认 false)
  - `match_path` (boolean, 可选, 默认 false)
  - `regex` (boolean, 可选, 默认 false)
  - `type_filter` (enum, 可选): `all` | `files` | `folders`

### 2. `everything_find_files`
无需记忆高级语法的结构化文件查找工具。
- **参数**：
  - `name` (string, 可选): 文件名关键字或通配符（如 `report` 或 `*.log`）
  - `extension` (string, 可选): 扩展名（如 `pdf` 或 `ts;tsx;js`）
  - `directory` (string, 可选): 限定搜索目录（如 `D:\Projects`）
  - `min_size` / `max_size` (string, 可选): 大小范围（如 `1MB`, `500KB`, `2GB`）
  - `modified_after` / `modified_before` (string, 可选): 修改日期（如 `today`, `yesterday`, `2026-01-01`）
  - `max_results` (number, 可选, 默认 30)

### 3. `everything_find_folders`
专用于查找目录路径的辅助工具。
- **参数**：
  - `name` (string, 可选): 文件夹名称关键字
  - `parent_directory` (string, 可选): 父目录路径限制
  - `max_results` (number, 可选, 默认 30)

### 4. `everything_get_file_info`
查询具体文件/文件夹的完整元数据。
- **参数**：
  - `path` (string, 必填): 目标文件的完整物理路径

### 5. `everything_preview_file`
预览搜索命中的文本文件前 N 行。
- **参数**：
  - `path` (string, 必填): 文件完整路径
  - `start_line` (number, 可选, 默认 1)
  - `max_lines` (number, 可选, 默认 50, 最大 200)

### 6. `everything_status`
检测 Everything 运行状态、服务延迟与索引条目总数。

---

## 💡 Everything 搜索常用语法速查

| 语法示例 | 说明 |
| :--- | :--- |
| `foo bar` | 查找名称中同时包含 `foo` 和 `bar` 的项目 |
| `foo \| bar` | 查找包含 `foo` 或 `bar` 的项目 |
| `!foo` | 查找不包含 `foo` 的项目 |
| `ext:ts;tsx;js` | 查找指定扩展名文件 |
| `size:>100MB` | 查找文件体积大于 100MB 的文件 |
| `size:1MB..10MB` | 查找体积在 1MB 至 10MB 之间的文件 |
| `dm:today` | 查找今天修改过的项目（`yesterday`, `thisweek`, `thismonth`, `thisyear`） |
| `dm:2026-01-01..2026-06-01` | 指定修改日期区间 |
| `parent:D:\Projects` | 限制在直接子目录下 |
| `exact:"C:\Windows\notepad.exe"` | 精确匹配完整路径 |

---

## 🛠️ 本地开发与构建

```bash
# 安装依赖
npm install

# 运行测试套件（连接本地运行的 Everything 进行端到端测试）
npm test

# 编译构建为 JavaScript (dist/)
npm run build

# 本地以开发模式运行 MCP Server
npm run dev
```

---

## 📄 开源许可与声明 (License)

本项目采用 [MIT License](LICENSE) 许可协议。
Everything 及其相关商标和知识产权归属于 voidtools (David Carpenter)。
