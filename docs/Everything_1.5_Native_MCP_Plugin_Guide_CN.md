# Everything 1.5 原生 MCP 插件开发与使用指南 (Native MCP Plugin)

> **设计参考**：本项目参考 [voidtools/http_server](https://github.com/voidtools/http_server) 与 [voidtools/etp_server](https://github.com/voidtools/etp_server) 的官方原生插件架构，将 Model Context Protocol (MCP) 核心服务以 **C 语言原生 DLL 插件 (`mcp_server64.dll`)** 形式直接运行于 **Everything 1.5 进程内部**。

---

## 🌟 原生插件与传统独立服务的区别

| 对比维度 | 传统独立外部服务 | Everything 原生 MCP 插件 (`mcp_server64.dll`) |
| :--- | :--- | :--- |
| **运行位置** | 独立的 Node.js / Python 进程 | **直接嵌入在 Everything.exe 内部运行** |
| **通信机制** | HTTP REST API / TCP 端口 (如 8088) | **原生内存指针调用 (`db_query_*`) + Windows 命名管道** |
| **网络端口** | 需要开启并占用本地 TCP 端口 | **零 TCP 端口暴露，纯本地命名管道 (`\\.\pipe\EverythingMCP`)** |
| **检索延迟** | 包含 HTTP 编解码与网络栈传输损耗 | **毫秒级零拷贝内存检索** |
| **插件管理** | 需要在外部启动与维护进程生命周期 | **随 Everything 主程序一同启停、在选项菜单统一管理** |

---

## 🏛️ 插件架构与工作原理

```
[ AI 客户端 (Claude Desktop / Cursor / Cline) ]
                      │ (Stdio 协议)
                      ▼
       [ 极简桥接器 / 原生管道直连 ]
                      │ (Windows Named Pipe: \\.\pipe\EverythingMCP)
                      ▼
┌────────────────────────────────────────────────────────┐
│ Everything.exe 进程空间                                 │
│                                                        │
│  ┌──────────────────────────────────────────────────┐  │
│  │ mcp_server64.dll (原生 MCP 插件)                   │  │
│  │                                                  │  │
│  │ 1. 导出 everything_plugin_proc 标准接口            │  │
│  │ 2. 监听 \\.\pipe\EverythingMCP 命名管道           │  │
│  │ 3. 内存直接调用 g_api->db_query_search() 极速查询  │  │
│  │ 4. 输出标准 JSON-RPC 2.0 (MCP 2024-11-05) 响应    │  │
│  └──────────────────────┬───────────────────────────┘  │
│                         │ (宿主 API 内存指针)          │
│  ┌──────────────────────▼───────────────────────────┐  │
│  │ Everything 核心索引引擎 (NTFS / USN / ReFS)        │  │
│  └──────────────────────────────────────────────────┘  │
└────────────────────────────────────────────────────────┘
```

---

## 🛠️ 插件安装与启用步骤

### 1. 复制插件 DLL
将编译生成的 `dist-plugin/mcp_server64.dll` 复制到 Everything 的 `Plugins` 目录下：
- 例如：`D:\Set\Everything\Everything\Plugins\mcp_server64.dll`
- 或系统默认目录：`C:\Program Files\Everything\Plugins\mcp_server64.dll`

### 2. 重启 Everything
1. 点击 Everything 菜单：`文件 (File) -> 退出 (Exit)`。
2. 重新启动 Everything。

### 3. 确认插件状态
1. 进入 `工具 (Tools) -> 选项 (Options)`。
2. 点击左侧 `插件 (Plug-ins)` 标签页。
3. 列表中将显示 **`Model Context Protocol (MCP) Server (mcp_server64.dll)`**。
4. 插件启动后，将自动在系统后台建立命名管道：`\\.\pipe\EverythingMCP`。

---

## 🤖 接入 AI 客户端配置

由于 Claude Desktop、Cursor 等工具目前普遍采用标准 Stdio 启动子进程，我们提供了一个极简高效的 Stdio <-> Named Pipe 转发桥（无重度运行时开销）：

### Claude Desktop 配置 (`claude_desktop_config.json`)
```json
{
  "mcpServers": {
    "everything-native": {
      "command": "node",
      "args": [
        "D:\\Set\\Everything\\EverythingPlugin\\Everything MCP\\bridge\\everything-pipe-bridge.ts"
      ]
    }
  }
}
```

### Cursor / Cline 配置
```json
{
  "mcpServers": {
    "everything-native": {
      "command": "node",
      "args": [
        "D:/Set/Everything/EverythingPlugin/Everything MCP/bridge/everything-pipe-bridge.ts"
      ]
    }
  }
}
```

---

## ⚙️ 源码编译指南

本项目原生插件采用标准 C99 编写，零第三方库依赖（仅链接 Windows 系统 `kernel32` 和 `user32`）：

### 使用 GCC (MinGW64) 编译：
```cmd
cd native-plugin
build.bat
```
或直接调用命令行：
```cmd
gcc -std=c99 -shared -O3 -Wall -Wextra -o "../dist-plugin/mcp_server64.dll" plugin_main.c mcp_server.c -lkernel32 -luser32
```

### 使用 CMake 编译：
```cmd
cd native-plugin
mkdir build && cd build
cmake ..
cmake --build . --config Release
```
