# 软件架构设计文档（SAD）

> Software Architecture Document

| 属性         | 值            |
| ------------ | ------------- |
| **文档编号** | SAD-001       |
| **项目名称** | NestPanes     |
| **版本**     | v0.1          |
| **状态**     | 草稿          |
| **作者**     | <!-- 作者 --> |
| **最后更新** | 2026-03-10    |

---

## 一、引言

### 1.1 目的

本文档描述 NestPanes 的软件架构设计，为开发者提供系统整体结构、模块划分、技术选型和关键设计决策的参考。

### 1.2 范围

本文档覆盖 NestPanes 应用的 `app/` 目录下所有自定义代码的架构设计，以及 Godot Engine 自身引擎代码的改动部分。

---

## 二、架构概述

### 2.1 架构目标与原则

| 原则                | 说明                                                                      |
| ------------------- | ------------------------------------------------------------------------- |
| **基于 Godot 架构** | 充分利用 Godot 的场景树、信号、节点等核心机制，遵循引擎约定               |
| **模块化**          | 功能按模块（app_modules）独立组织，模块可独立开发与测试                   |
| **窗格抽象**        | 所有可嵌入标签页的内容抽象为 PaneBase，通过 PaneFactory 注册/创建         |
| **平台抽象**        | 文件系统操作通过 FileSystemAccess 抽象层，平台实现隔离到 `platform/` 目录 |
| **可扩展**          | 新功能以新的 Pane 类型和 AppModule 形式加入，无需修改核心框架             |
| **最小依赖**        | 禁用所有不需要的 Godot 模块（3D、物理、GDScript、网络等），保持精简       |

### 2.2 技术选型

| 类别     | 技术      | 版本       | 选型理由                                 |
| -------- | --------- | ---------- | ---------------------------------------- |
| 编程语言 | C++       | C++17      | Godot Engine 原生语言，高性能            |
| UI 框架  | Godot GUI | 4.6-stable | 成熟的 UI 控件体系、场景树管理、信号系统 |
| 构建系统 | SCons     | 4.x        | Godot 官方构建系统，与引擎完全集成       |
| 平台 API | Win32     | —          | 原生文件系统操作、系统图标获取           |
| 版本控制 | Git       | —          | —                                        |
| 测试框架 | doctest   | —          | Godot 内置的 C++ 测试框架                |

---

## 三、系统架构

### 3.1 系统架构图

### 3.2 层次说明

| 层次                                       | 职责               | 关键组件                                                                                                                    |
| ------------------------------------------ | ------------------ | --------------------------------------------------------------------------------------------------------------------------- |
| **基础 UI** (`app/gui`)                    | UI 框架和布局管理  | LayoutManager, PaneManager, AppTabContainer, MultiSplitContainer, PaneFactory, PaneBase                                     |
| **基础工具及定义** (`app/app_core`)        | 平台抽象和基础类型 | AppPaths, FileSystemAccess, FileInfo                                                                                        |
| **平台** (`app/app_core/platform/windows`) | Windows 平台实现   | FileSystemAccessWindows                                                                                                     |
| **应用模块** (`app/app_modules`)           | 具体业务功能实现   | file_management (FilePane, NavigationPane, FileSystemTree, FileContextMenu), text_editing (TextEditor), workspaces (待实现) |
| **设置** (`app/settings`)                  | 应用配置管理       | AppSettings, SettingsPane                                                                                                   |
| **主题** (`app/themes`)                    | 外观和主题管理     | AppTheme, AppThemeManager, AppIcons, AppFonts, theme_classic, theme_modern                                                  |
| **翻译** (`app/translations`)              | 国际化支持         | AppTranslation, PO 文件                                                                                                     |

---

## 四、模块设计

`app/` 目录下的代码分为两个层面：

- **应用框架**：`gui/`、`app_core/`、`settings/`、`themes/`、`translations/` 等目录，提供窗格系统、布局管理、平台抽象、设置管理、主题等基础设施，不包含具体业务逻辑。
- **应用模块**（`app_modules/`）：基于应用框架构建的具体业务功能模块。每个子目录是一个独立模块，包含业务逻辑和对应的 GUI 组件。通过继承 `PaneBase` 并注册到 `PaneFactory` 来接入框架。

### 4.1 模块总览

### 4.2 应用框架

### 4.3 应用模块（app_modules/）

---

## 五、数据

### 5.1 数据存储

| 数据     | 存储方式        | 位置                                    | 格式                |
| -------- | --------------- | --------------------------------------- | ------------------- |
| 用户配置 | Resource (tres) | `%APPDATA%/NestPanes/app_settings.tres` | Godot Resource 格式 |
| 布局数据 | ConfigFile      | `%APPDATA%/NestPanes/app_layouts.cfg`   | INI-like            |
