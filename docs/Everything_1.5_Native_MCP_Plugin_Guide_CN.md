# Everything 1.5 原生 MCP 插件开发与安装指南

本项目完全遵循 [voidtools/http_server](https://github.com/voidtools/http_server) 和 [voidtools/etp_server](https://github.com/voidtools/etp_server) 官方原生插件的设计规范，将 Model Context Protocol (MCP) 核心服务封装为 **C 语言原生 DLL 插件 (`mcp_server64.dll`)** 与 **一键安装器 (`Everything-MCP-Server-Setup.exe`)**，直接嵌入运行在 Everything 1.5 进程内部。

---

## 🌟 原生插件与传统外部服务的区别

| 对比维度 | 传统外部独立服务 | Everything 官方规范原生插件 (`mcp_server64.dll`) |
| :--- | :--- | :--- |
| **运行机制** | 独立的 Node.js / Python 外部进程 | **直接作为 DLL 模块加载进 Everything.exe 内部** |
| **API 调用** | 经由 HTTP 端口请求 | **直接调用 Everything 内部导出的 `db_query_*` 内存指针** |
| **图形化配置** | 依靠外部环境变量或配置文件 | **原生嵌入 Everything 的 `工具 -> 选项 -> 插件 -> MCP Server`** |
| **安装方式** | 需配置 Node 环境并手动运行服务 | **提供官方风格一键安装器 `Setup.exe` 或将 DLL 放入 `Plugins\`** |
| **通信通道** | 仅本地 TCP 端口 | **本地 Windows 命名管道 (`\\.\pipe\EverythingMCP`) + HTTP/SSE (`8765`)** |

---

## 📦 插件安装方式（两种方式可选）

### 方式一：一键安装程序（推荐）
1. 运行 `dist-plugin/Everything-MCP-Server-Setup.exe`。
2. 安装器会自动扫描已安装的 Everything 1.5 并提示确认。
3. 点击 **是 (Yes)** 完成自动部署。
4. 重启 Everything 1.5（菜单：`文件 -> 退出`，然后重新打开 Everything）。

### 方式二：手动复制 DLL
1. 将编译生成的 `dist-plugin/mcp_server64.dll` 复制到 Everything 的 `Plugins` 目录：
   - 默认安装路径：`C:\Program Files\Everything\Plugins\mcp_server64.dll`
   - 或便携版路径：`D:\Set\Everything\Everything\Plugins\mcp_server64.dll`
2. 在 Everything 菜单中点击 `文件 -> 退出`。
3. 重新启动 Everything。

---

## ⚙️ Everything 内置配置选项

安装完成后，打开 Everything：
1. 点击菜单：`工具 (Tools) -> 选项 (Options)`。
2. 在左侧列表的 `插件 (Plug-ins)` 分类下，将出现全新的 **`MCP Server`** 属性页。
3. 可配置项包括：
   - ☑️ **Enable MCP Server**：开启或禁用 MCP 检索服务。
   - **Named Pipe Name**：本地 Windows 命名管道地址（默认 `\\.\pipe\EverythingMCP`）。
   - **HTTP / SSE Port**：HTTP 与 Server-Sent Events 服务端口（默认 `8765`）。
   - ☑️ **Allow file content preview**：是否允许 AI 客户端调用 `everything_preview_file` 读取文件文本预览。
   - **Default Max Results**：默认最大搜索结果返回条数（默认 `100`）。
   - **Restore Defaults**：一键恢复默认配置。

---

## 🤖 接入 AI 客户端配置

### 1. Claude Desktop 配置 (`claude_desktop_config.json`)
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

### 2. Cursor / Cline / Windsurf 配置
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

## 🛠️ 源码构建

### 一键构建全部产物（DLL 插件 + 安装器）：
```cmd
build.bat
```

产物将输出至 `dist-plugin/` 目录：
- `dist-plugin/mcp_server64.dll`（原生插件 DLL，约 160 KB）
- `dist-plugin/Everything-MCP-Server-Setup.exe`（官方风格一键安装器，约 140 KB）
