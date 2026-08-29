# Everything 1.5 插件系统与 SDK 开发者文档（中文存档）

> **文档来源与背景**：本开发文档基于 voidtools 官方论坛专帖 [Everything 1.5 Plug-ins (Topic #9799)](https://www.voidtools.com/forum/viewtopic.php?f=12&t=9799)、[Everything 1.5 Plugin SDK (Topic #16535)](https://www.voidtools.com/forum/viewtopic.php?t=16535) 以及 voidtools 官方技术支持文档整理与翻译存档。

---

## 目录
1. [架构概述与设计初衷](#1-架构概述与设计初衷)
2. [官方核心插件介绍](#2-官方核心插件介绍)
3. [插件安装、管理与排错](#3-插件安装管理与排错)
4. [插件 SDK 与接口规范 (Plugin SDK)](#4-插件-sdk-与接口规范-plugin-sdk)
   - [4.1 核心导出入口函数](#41-核心导出入口函数)
   - [4.2 插件消息生命周期 (Plugin Messages)](#42-插件消息生命周期-plugin-messages)
   - [4.3 Everything 宿主导出 API 详解](#43-everything-宿主导出-api-详解)
5. [Everything SDK 与 IPC 通信机制](#5-everything-sdk-与-ipc-通信机制)
   - [5.1 命名管道 (Named Pipe) IPC](#51-命名管道-named-pipe-ipc)
   - [5.2 隐藏窗口焦点获取机制](#52-隐藏窗口焦点获取机制)
   - [5.3 实例名称与向下兼容性配置](#53-实例名称与向下兼容性配置)
6. [HTTP Server 插件 REST / JSON API 规范](#6-http-server-插件-rest--json-api-规范)
   - [6.1 查询接口与参数说明](#61-查询接口与参数说明)
   - [6.2 返回 JSON 结构与时间戳转换](#62-返回-json-结构与时间戳转换)
7. [版本发布与演进历程](#7-版本发布与演进历程)
8. [参考资源与代码仓库](#8-参考资源与代码仓库)

---

## 1. 架构概述与设计初衷

在 **Everything 1.5** 架构体系中，官方对软件的核心设计理念进行了重要重构：
- **解耦服务模块**：自 Everything 1.5.0.1351a 起，所有的服务功能（包括 HTTP Server、ETP/FTP Server 以及 Everything Server）已全量从主程序本体中抽离，转为以独立 **DLL 插件** 的形式提供。
- **轻量化与模块化**：主程序仅保留核心索引引擎、IPC 接口和 GUI 视图，第三方或官方插件通过标准的 C 风格导出函数与消息循环接入 Everything 宿主。
- **增强稳定性**：插件独立运行于扩展生命周期中，且主程序提供命令行安全模式（Safe Mode），防止故障插件导致主程序崩溃。

---

## 2. 官方核心插件介绍

| 插件名称 | DLL 文件名 | 主要功能 | 适用场景 |
| :--- | :--- | :--- | :--- |
| **HTTP Server** | `http_server64.dll` / `http_server32.dll` | 提供 Web 界面与 REST/JSON API | 网页端搜索、第三方工具/脚本/MCP 远程或本地快速查询文件 |
| **ETP/FTP Server** | `etp_server64.dll` / `etp_server32.dll` | 基于 FTP/ETP 协议的文件检索与传输 | Everything 客户端远程互联、FTP 客户端访问文件索引 |
| **Everything Server** | `everything_server64.dll` / `32.dll` | 企业级/局域网索引快照服务器（需 Site License） | 多用户中央索引共享、大并发只读快照检索 |

---

## 3. 插件安装、管理与排错

### 3.1 插件安装方式
1. **安装包方式**：运行官方提供的 `Everything-*-Setup.exe`，安装程序会自动检测 Everything 路径并完成注册。
2. **便携式/手动安装**：
   - 下载便携 ZIP 包，解压得到对应架构的 `.dll` 文件（如 `http_server64.dll`）。
   - 将 DLL 放入 Everything 安装根目录下的 `Plugins` 文件夹（例如 `C:\Program Files\Everything\Plugins\` 或 `D:\Set\Everything\Everything\Plugins\`）。
   - 退出 Everything（`文件 -> 退出`），重新启动 Everything。

### 3.2 插件启用与配置界面
- **官方插件**：默认自动加载并启用。
- **第三方插件**：
  1. 打开 Everything，进入菜单 `工具 (Tools) -> 选项 (Options)`。
  2. 点击左侧 `插件 (Plug-ins)` 标签页。
  3. 选中对应插件，勾选 `启用插件 (Enable plugin)` 并进行个性化参数配置。

### 3.3 配置文件 `Plugins.ini` 结构
Everything 将所有插件的运行配置统一集中在 `%APPDATA%\Everything\Plugins.ini`（便携版位于软件根目录下）。
示例配置：
```ini
; 修改此文件前请确保 Everything 已完全退出
[http_server64.dll]
enabled=1
port=8088
bindings=localhost;
allow_file_download=1
logging_enabled=1
log_file_name=D:\Set\Everything\Everything\Logs\HTTP_Server_Log1.5.txt
items_per_page=32
allow_query_access=0
allow_disk_access=0
default_sort=3
default_sort_ascending=0
```

### 3.4 安全模式排错 (Safe Mode)
如果某个插件异常导致 Everything 无法正常启动或崩溃，可以使用 `-safe-mode` 命令行参数跳过所有插件启动：
```cmd
Everything.exe -safe-mode
```

---

## 4. 插件 SDK 与接口规范 (Plugin SDK)

### 4.1 核心导出入口函数
所有 Everything 1.5 插件 DLL 必须导出单一标准入口函数 `everything_plugin_proc`：

```c
#include <windows.h>

#define EVERYTHING_PLUGIN_API __stdcall

__declspec(dllexport) void * EVERYTHING_PLUGIN_API everything_plugin_proc(DWORD msg, void *data)
{
    // msg: 对应 EVERYTHING_PLUGIN_PM_* 消息常量
    // data: 根据消息类型的输入数据结构体指针
    switch (msg)
    {
        case EVERYTHING_PLUGIN_PM_INIT:
            // 接收宿主函数导出表指针
            break;
        case EVERYTHING_PLUGIN_PM_GET_NAME:
            return L"My Custom Plugin";
        case EVERYTHING_PLUGIN_PM_GET_VERSION:
            return L"1.0.0.1";
        case EVERYTHING_PLUGIN_PM_START:
            // 启动插件工作线程/服务
            return (void *)1;
        case EVERYTHING_PLUGIN_PM_STOP:
            // 停止服务
            return (void *)1;
        // 其他消息...
    }
    
    // 未处理的消息返回 0
    return (void *)0;
}
```

### 4.2 插件消息生命周期 (Plugin Messages)

| 消息常量 (`msg`) | 触发时机与作用 | 返回值要求 |
| :--- | :--- | :--- |
| `EVERYTHING_PLUGIN_PM_INIT` | 插件加载初始化，传入宿主 API 表 `everything_plugin_api_t*` | 成功返回非 0 |
| `EVERYTHING_PLUGIN_PM_GET_PLUGIN_VERSION` | 获取插件 SDK 版本号兼容性校验 | 返回 SDK 版本常量 |
| `EVERYTHING_PLUGIN_PM_GET_NAME` | 获取插件显示名称 | 返回 `const wchar_t*` |
| `EVERYTHING_PLUGIN_PM_GET_DESCRIPTION` | 获取插件描述信息 | 返回 `const wchar_t*` |
| `EVERYTHING_PLUGIN_PM_GET_AUTHOR` | 获取插件作者信息 | 返回 `const wchar_t*` |
| `EVERYTHING_PLUGIN_PM_GET_VERSION` | 获取插件版本字符串 | 返回 `const wchar_t*` |
| `EVERYTHING_PLUGIN_PM_GET_LINK` | 获取插件主页或反馈 URL | 返回 `const wchar_t*` |
| `EVERYTHING_PLUGIN_PM_START` | 插件启用/服务启动 | 成功返回 1 |
| `EVERYTHING_PLUGIN_PM_STOP` | 插件禁用/服务停止 | 成功返回 1 |
| `EVERYTHING_PLUGIN_PM_KILL` | 插件卸载前清理资源 | 0 |
| `EVERYTHING_PLUGIN_PM_UNINSTALL` | 插件被用户删除卸载 | 0 |
| `EVERYTHING_PLUGIN_PM_ADD_OPTIONS_PAGES`| 注册插件在 Everything 选项中的自定义设置页面 | 选项页结构指针 |
| `EVERYTHING_PLUGIN_PM_LOAD_OPTIONS_PAGE`| 加载设置页面 UI 数据 | 0 |
| `EVERYTHING_PLUGIN_PM_SAVE_OPTIONS_PAGE`| 用户点击确定/应用时保存设置 | 0 |
| `EVERYTHING_PLUGIN_PM_SAVE_SETTINGS` | 请求插件持久化配置至 `Plugins.ini` | 0 |

### 4.3 Everything 宿主导出 API 详解
Everything 在 `PM_INIT` 中会向插件注入丰富的核心引擎接口指针：

1. **数据库与检索接口 (`db_*`)**：
   - `db_query_create()`: 创建搜索查询上下文。
   - `db_query_search(q, search_string, flags)`: 执行毫秒级极速全文索引匹配。
   - `db_query_get_result_count(q)`: 获取查询命中的结果总量。
   - `db_query_get_result_name(q, index)`: 获取指定结果项的文件名。
   - `db_query_get_result_path(q, index)`: 获取指定结果项的所在文件夹完整路径。
   - `db_query_sort(q, sort_type, ascending)`: 对结果集排序。
   - `db_snapshot_*`: 建立只读数据库快照，支持无锁并发高吞吐检索。
   - `db_journal_*`: 监听 NTFS USN / ReFS / 文件变更日志。

2. **内存与字符串处理 (`mem_*`, `utf8_*`, `wchar_*`)**：
   - 宿主内存池分配与释放（`mem_alloc`, `mem_calloc`, `mem_free`）。
   - 高性能 UTF-8 缓冲区安全操作（`utf8_buf_init`, `utf8_buf_printf`, `utf8_buf_escape_html`, `utf8_buf_path_canonicalize` 等）。

3. **网络与套接字抽象 (`network_*`, `os_winsock_*`)**：
   - 跨平台/统一的非阻塞 Winsock 事件绑定、TCP KeepAlive、TCP_NODELAY 设置。

4. **配置与国际化 (`config_*`, `ini_*`, `localization_*`)**：
   - 直接存取 `Plugins.ini` 的键值对配置，支持多语言本地化资源字符串。

---

## 5. Everything SDK 与 IPC 通信机制

### 5.1 命名管道 (Named Pipe) IPC
Everything 1.5 引入了现代化的 Windows Named Pipe（命名管道）作为进程间通信基础，相比 1.4 的老旧窗口消息广播更加高效、安全：
- **默认命名管道地址**：`\\.\PIPE\Everything IPC`
- **带实例名管道地址**：`\\.\PIPE\Everything IPC (<InstanceName>)`（例如 `\\.\PIPE\Everything IPC (1.5a)`）
- **服务管道地址**：`\\.\pipe\Everything Service`

### 5.2 隐藏窗口焦点获取机制
Everything 1.5 内置了一个特殊的隐藏窗口，其窗口文本时刻同步着主界面当前选中的高亮项目路径：
- **窗口类名 (Window Class)**：`EVERYTHING_RESULT_LIST_FOCUS`
- **使用方式**：第三方程序通过 `FindWindowW(L"EVERYTHING_RESULT_LIST_FOCUS", NULL)` 获取句柄，使用 `GetWindowTextW` 获取当前用户选中的文件。

### 5.3 实例名称与向下兼容性配置
如果第三方工具基于 **Everything 1.4 SDK**（依赖旧版窗口消息或非实例管道）：
1. 打开 Everything 1.5，进入 `工具 (Tools) -> 选项 (Options)`。
2. 点击左侧 `高级 (Advanced)`。
3. 搜索 `alpha`，找到 `alpha_instance` 设置项。
4. 将值设为 `false` 并确认重启，即可使 Everything 1.5 以默认实例运行，完全兼容 1.4 SDK。

---

## 6. HTTP Server 插件 REST / JSON API 规范

HTTP Server 插件是外部应用程序（如 Python 脚本、Node.js 服务、MCP 服务器）与 Everything 进行极速检索集成的最通用、跨平台的方案。

### 6.1 查询接口与参数说明
- **请求端点**：`GET http://<host>:<port>/?<parameters>`
- **核心查询参数表**：

| 参数名 | 简写 | 类型 | 说明 | 示例 |
| :--- | :--- | :--- | :--- | :--- |
| `search` | `s` | string | 搜索表达式（支持完整 Everything 语法） | `search=test.pdf` |
| `json` | - | int | 必须设为 `1`，表示返回 JSON 格式 | `json=1` |
| `count` | `c` | int | 限制返回条目数（分页大小） | `count=50` |
| `offset` | `o` | int | 偏移起始索引（分页游标，0 索引） | `offset=0` |
| `sort` | - | string | 排序字段：`name`, `path`, `size`, `extension`, `date_modified`, `date_created` | `sort=size` |
| `ascending` | - | int | `1` 升序，`0` 降序 | `ascending=0` |
| `case` | - | int | `1` 开启区分大小写，`0` 忽略 | `case=1` |
| `wholeword` | `ww` | int | `1` 全字匹配，`0` 模糊 | `wholeword=1` |
| `path` | - | int | `1` 匹配包含路径，`0` 仅匹配文件名 | `path=1` |
| `regex` | - | int | `1` 启用正则表达式搜索 | `regex=1` |
| `path_column` | - | int | `1` 在返回结果中包含 `path` 字段 | `path_column=1` |
| `size_column` | - | int | `1` 在返回结果中包含 `size` 字段 (字节) | `size_column=1` |
| `date_modified_column` | - | int | `1` 在返回结果中包含 `date_modified` 字段 | `date_modified_column=1` |
| `date_created_column` | - | int | `1` 在返回结果中包含 `date_created` 字段 | `date_created_column=1` |
| `attributes_column` | - | int | `1` 在返回结果中包含文件属性掩码 | `attributes_column=1` |

### 6.2 返回 JSON 结构与时间戳转换
调用 `GET /?search=Everything.exe&json=1&count=2&path_column=1&size_column=1&date_modified_column=1` 返回样例：
```json
{
  "totalResults": 6,
  "results": [
    {
      "type": "file",
      "name": "Everything.exe",
      "path": "D:\\Set\\Everything\\Everything",
      "size": "4520960",
      "date_modified": "134324570024510822"
    }
  ]
}
```

#### Windows FILETIME 时间戳转换公式
返回的 `date_modified` 为 64 位整数（自 1601-01-01 UTC 以来的 100 纳秒间隔）：
- **Unix 毫秒换算公式**：
  $$\text{UnixTime (ms)} = \frac{\text{FILETIME} - 116444736000000000}{10000}$$
- **JavaScript / TypeScript 代码实现**：
  ```typescript
  function fileTimeToDate(filetimeStr: string): Date {
    const filetime = BigInt(filetimeStr);
    const unixMs = Number((filetime - 116444736000000000n) / 10000n);
    return new Date(unixMs);
  }
  ```

---

## 7. 版本发布与演进历程

- **2021-03-13**：Everything 1.5 Alpha 首次提出插件体系结构，将 ETP/FTP/HTTP 服务器移至独立插件。
- **2023-05 ~ 2023-06**：发布 1.0.0.0 与 1.0.0.1 版本，加入标准 Windows 安装包并修复 x86/x64 架构兼容性。
- **2023-10-29 (v1.0.2.2)**：修复 HTTP Server 名称排序记忆失效的问题。
- **2024-03-18 (v1.0.1.2)**：修复 ETP Server 中 EPRT 端口问题，增加 XPWD 别名。
- **2024-05-19 (v1.0.3.3)**：修复 HTTP Server 访问根斜杠路径（例如 `/home`）的解析异常。
- **2024-11-29**：Everything 1.5.0.1384a 正式配套发布 Everything 1.5 SDK。
- **2025-05-26**：官方正式开放 HTTP Server 及 ETP Server 开源源代码，并在论坛发布完整的 Everything 1.5 Plugin SDK。更新 voidtools PTY LTD 代码签名证书与强化加密。
- **2025-06-09 (v1.0.1.4)**：修复 FTP/ETP 中 MDTM、MLSD 和 MLST 的时区偏移问题。
- **2026-02-05 (v1.0.2.3)**：新增 Everything Server 索引快照缓存超时与最大快照数控制。
- **2026-04-30 (v1.0.3.4)**：新增站点许可证注册系统与多国语言本地化支持。
- **2026-05-14 (v1.0.4.5 & v1.0.2.5)**：全面适配 Everything 1.5 Beta 版。

---

## 8. 参考资源与代码仓库

- [voidtools 官方论坛 Plugin 讨论主帖 (Topic 9799)](https://www.voidtools.com/forum/viewtopic.php?f=12&t=9799)
- [Everything 1.5 Plugin SDK 官方指南 (Topic 16535)](https://www.voidtools.com/forum/viewtopic.php?t=16535)
- [Everything 1.5 SDK 官方发布帖 (Topic 12402 & 15853)](https://www.voidtools.com/forum/viewtopic.php?t=12402)
- [GitHub: voidtools/http_server 官方开源仓库](https://github.com/voidtools/http_server)
- [GitHub: voidtools/etp_server 官方开源仓库](https://github.com/voidtools/etp_server)
