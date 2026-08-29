# Everything MCP (Native Plugin & MCP Server)

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

本项目提供了对接 [voidtools Everything](https://www.voidtools.com/) 的 Model Context Protocol (MCP) 解决方案，包含两种运行模式：

1. **原生插件模式 (`mcp_server64.dll`)**：参考 [voidtools/http_server](https://github.com/voidtools/http_server) 和 [voidtools/etp_server](https://github.com/voidtools/etp_server) 官方架构，以 C 语言原生 DLL 形式直接运行在 `Everything.exe` 进程内部，零网络端口暴露，通过 Windows 命名管道（`\\.\pipe\EverythingMCP`）提供极速的内存级搜索服务。
2. **独立服务模式 (`everything-mcp-server`)**：基于 Node.js / TypeScript，通过 Everything HTTP API 与正在运行的 Everything 实例通信。

---

## 🌟 运行模式对比与选择

| 模式 | 运行形式 | 通信方式 | 适用场景 |
| :--- | :--- | :--- | :--- |
| **原生插件模式 (推荐)** | `mcp_server64.dll` 放入 `Everything\Plugins\` | 内存指针 + 本地命名管道 (`\\.\pipe\EverythingMCP`) | 追求零网络端口、纯原生进程内直连、低延迟与高安全性 |
| **独立服务模式** | 独立的 Node.js MCP 进程 | HTTP REST API (`http://127.0.0.1:8088`) | 跨机器/容器远程检索，或希望独立管理服务进程 |

---

## 🚀 模式一：原生插件部署（推荐）

### 1. 安装插件
将 `dist-plugin/mcp_server64.dll` 复制到 Everything 的 `Plugins` 目录（例如 `D:\Set\Everything\Everything\Plugins\` 或 `C:\Program Files\Everything\Plugins\`）。

### 2. 重启 Everything
在 Everything 菜单中点击 `文件 -> 退出`，重新打开 Everything。在 `工具 -> 选项 -> 插件` 即可看到 `Model Context Protocol (MCP) Server` 已自动加载并在后台监听 `\\.\pipe\EverythingMCP`。

### 3. 客户端配置 (Claude Desktop / Cursor)
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

---

## 🚀 模式二：独立服务部署

### 1. 前置要求
1. 在 Everything 中启用 HTTP 服务器（`工具 -> 选项 -> 插件 -> HTTP 服务器`，默认端口 `8088`）。
2. 安装 Node.js（>= 18.0.0）。

### 2. 客户端配置 (Claude Desktop / Cursor)
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

---

## 🔧 支持的 MCP 工具

- `everything_search`: 通用搜索，支持完整 Everything 语法（通配符、`ext:`、`size:`、`dm:`、`regex:` 等）、排序与分页。
- `everything_find_files`: 结构化查找文件（按文件名、扩展名、目录、大小与日期筛选）。
- `everything_find_folders`: 结构化查找目录。
- `everything_get_file_info`: 获取文件在 Everything 数据库中的索引信息与本地属性。
- `everything_preview_file`: 预览文本文件前 N 行（内置二进制文件检测）。
- `everything_status`: 检查服务运行状态与已索引项目总数。

---

## 📁 项目结构

```
📁 Everything MCP/
├── 📁 native-plugin/                    # C 语言原生 Everything 1.5 插件源码
│   ├── 📄 everything_plugin.h           # Everything 1.5 Plugin SDK 规范头文件
│   ├── 📄 mcp_server.h & .c             # 进程内 MCP JSON-RPC 2.0 与命名管道实现
│   ├── 📄 plugin_main.c                 # 插件生命周期与 everything_plugin_proc
│   ├── 📄 build.bat                     # Windows 一键编译脚本
│   └── 📄 CMakeLists.txt                # CMake 构建脚本
├── 📁 dist-plugin/                      # 编译生成的 mcp_server64.dll
├── 📁 bridge/                           # 极简 Stdio <-> Named Pipe 高速桥接
│   └── 📄 everything-pipe-bridge.ts
├── 📁 src/                              # Node.js 独立 MCP 服务源码
├── 📁 docs/                             # 开发文档与权威存档
│   ├── 📄 Everything_1.5_Plugins_and_SDK_CN.md
│   └── 📄 Everything_1.5_Native_MCP_Plugin_Guide_CN.md
├── 📄 package.json
└── 📄 README.md
```

---

## 🛠️ 本地编译与构建

### 编译原生 C 插件
```cmd
cd native-plugin
build.bat
```

### 编译 TypeScript 服务
```bash
npm install
npm test
npm run build
```

---

## 📄 版权与声明

- **Everything** 与 **Everything 搜索引擎** 归 **David Carpenter (voidtools)** 所有。
- 本项目为独立的社区开源实现，与 voidtools 无官方附属关系。
- 许可协议：[MIT License](LICENSE)。
