# Everything MCP (Native Plugin & MCP Server)

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

Model Context Protocol (MCP) Server for [Everything 1.5](https://www.voidtools.com/).

本项目参考 [voidtools/http_server](https://github.com/voidtools/http_server) 和 [voidtools/etp_server](https://github.com/voidtools/etp_server) 官方原生插件架构开发，将 Model Context Protocol (MCP) 服务作为 **Everything 1.5 原生 C 语言 DLL 插件 (`mcp_server64.dll`)** 运行于 Everything 进程内部，同时提供一键安装程序 (`Everything-MCP-Server-Setup.exe`) 与独立服务运行模式。

---

## 🌟 核心特性

- **官方原生插件架构**：实现 Everything 1.5 插件标准生命周期回调与动态函数绑定，直接调用内存中的 `db_query_*` 索引查询引擎。
- **Everything 选项菜单集成**：无缝集成至 Everything 的 `工具 -> 选项 -> 插件 -> MCP Server` 配置页，支持图形化管理服务开关、管道名称、端口、最大结果数等。
- **双传输通道**：支持 Windows 命名管道（`\\.\pipe\EverythingMCP`）与本地 HTTP/SSE（`http://127.0.0.1:8765/mcp`）。
- **完整 MCP 工具集**：提供搜索、定向文件/文件夹查找、元数据查询、安全文本预览、状态检测等 6 项标准 MCP 工具。

---

## 📦 插件安装与使用

### 方式一：一键安装程序（推荐）
1. 下载或编译生成的 `dist-plugin/Everything-MCP-Server-Setup.exe`。
2. 双击运行安装程序，点击确认完成部署。
3. 在 Everything 中点击 `文件 -> 退出`，然后重新打开 Everything 1.5。

### 方式二：手动复制 DLL
1. 将 `dist-plugin/mcp_server64.dll` 复制到 Everything 的 `Plugins` 目录（例如 `C:\Program Files\Everything\Plugins\` 或 `D:\Set\Everything\Everything\Plugins\`）。
2. 在 Everything 中点击 `文件 -> 退出` 并重新启动 Everything。

### 管理插件选项
在 Everything 主界面中，点击 `工具 (Tools) -> 选项 (Options) -> 插件 (Plug-ins) -> MCP Server` 即可实时调整配置。

---

## 🤖 接入 AI 客户端配置

### Claude Desktop (`claude_desktop_config.json`)
```json
{
  "mcpServers": {
    "everything": {
      "command": "node",
      "args": [
        "D:\\Set\\Everything\\EverythingPlugin\\Everything MCP\\bridge\\everything-pipe-bridge.ts"
      ]
    }
  }
}
```

### Cursor / Cline / Windsurf
```json
{
  "mcpServers": {
    "everything": {
      "command": "node",
      "args": [
        "D:/Set/Everything/EverythingPlugin/Everything MCP/bridge/everything-pipe-bridge.ts"
      ]
    }
  }
}
```

---

## 🔧 支持的 MCP 工具列表

| 工具名称 | 功能描述 |
| :--- | :--- |
| `everything_search` | 通用全文检索，支持通配符、`ext:`、`size:`、`dm:`、`regex:` 等完整语法 |
| `everything_find_files` | 结构化查找文件（按名称、扩展名、目录、大小与日期） |
| `everything_find_folders` | 结构化查找目录 |
| `everything_get_file_info` | 查询文件/目录在 Everything 中的索引属性与本地文件属性 |
| `everything_preview_file` | 文本文件安全预览（支持设置最大行数与字节数，自动检测二进制文件） |
| `everything_status` | 查看 Everything MCP 插件运行状态与配置详情 |

---

## 🛠️ 本地编译构建

### 一键构建原生插件与安装程序
```cmd
build.bat
```
构建生成的文件位于 `dist-plugin/`：
- `dist-plugin/mcp_server64.dll`
- `dist-plugin/Everything-MCP-Server-Setup.exe`

### 编译 TypeScript 独立服务
```bash
npm install
npm test
npm run build
```

---

## 📁 项目目录结构

```
📁 Everything MCP/
├── 📁 native-plugin/                    # C 语言原生 Everything 1.5 插件源码
│   ├── 📄 everything_plugin.h           # Everything 1.5 官方 SDK 头文件
│   ├── 📄 mcp_server.h & .c             # 进程内 MCP 协议、命名管道与 HTTP 引擎
│   ├── 📄 plugin_main.c                 # everything_plugin_proc 生命周期与选项 UI
│   ├── 📄 version.h                     # 版本定义
│   ├── 📄 mcp_server.def                # 导出定义
│   └── 📄 build.bat                     # 插件编译脚本
├── 📁 setup/                            # 官方风格一键安装程序源码
│   ├── 📁 src/
│   │   └── 📄 setup.c
│   └── 📄 build-setup.bat
├── 📁 dist-plugin/                      # 编译生成物 (DLL & Setup.exe)
├── 📁 bridge/                           # Stdio 到命名管道的高速桥接
│   └── 📄 everything-pipe-bridge.ts
├── 📁 src/                              # Node.js 独立 MCP 服务源码
├── 📁 docs/                             # 开发与安装指南文档
│   ├── 📄 Everything_1.5_Plugins_and_SDK_CN.md
│   └── 📄 Everything_1.5_Native_MCP_Plugin_Guide_CN.md
├── 📄 build.bat                         # 全量构建脚本
├── 📄 LICENSE
└── 📄 README.md
```

---

## 📄 版权与声明

- **Everything** 与 **Everything 搜索引擎** 归 **David Carpenter (voidtools)** 所有。
- 本项目为独立的开源实现，与 voidtools 无官方附属关系。
- 许可协议：[MIT License](LICENSE)。
