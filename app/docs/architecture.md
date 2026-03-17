# 软件架构设计文档（SAD）

> Software Architecture Document

| 属性         | 值            |
| ------------ | ------------- |
| **文档编号** | SAD-001       |
| **项目名称** | NestPanes     |
| **版本**     | v0.1          |
| **状态**     | 草稿          |
| **作者**     | <!-- 作者 --> |
| **最后更新** | 2026-03-17    |

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

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                              AppNode (根节点)                                    │
├────────────────────┬───────────────────────┬────────────────────────────────────┤
│   LayoutManager    │    PaneManager        │         AppThemeManager            │
│   (布局管理)        │    (窗格管理)          │         (主题管理)                 │
├────────────────────┴───────────────────────┴────────────────────────────────────┤
│                       应用模块层 (app/app_modules)                               │
│  ┌──────────────────┐ ┌──────────────────┐ ┌──────────────────┐                  │
│  │ file_management  │ │  text_editing    │ │   workspaces     │                  │
│  │ (文件管理)        │ │ (文本编辑)       │ │ (工作空间)        │                  │
│  │  FilePane        │ │  (待实现)        │ │  (待实现)         │                  │
│  │  NavigationPane  │ │                  │ │                  │                  │
│  │  FileSystem      │ │                  │ │                  │                  │
│  └──────────────────┘ └──────────────────┘ └──────────────────┘                  │
├─────────────────────────────────────────────────────────────────────────────────┤
│                          基础 UI 层 (app/gui)                                    │
│  ┌─────────────────┐ ┌───────────────────┐ ┌──────────────────┐                 │
│  │AppTabContainer  │ │MultiSplitContainer│ │   PaneFactory    │                 │
│  │ (标签页容器)     │ │ (拆分容器)         │ │ (窗格工厂)       │                  │
│  └────────┬────────┘ └───────────────────┘ └────────┬─────────┘                 │
│           │                                        │                            │
│           ▼                                        ▼                            │
│      PaneBase (窗格基类) ◄──────────────── 各模块 Pane 继承                       │
├─────────────────────────────────────────────────────────────────────────────────┤
│                       基础设施层                                                 │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐ ┌──────────────┐            │
│  │  app_core    │ │  settings    │ │   themes     │ │ translations │            │
│  │  AppPaths    │ │  AppSettings │ │  AppTheme    │ │AppTranslation│            │
│  │  FileInfo    │ │  SettingsPane│ │  AppIcons    │ │  PO 文件     │            │
│  │  FSAccess    │ │              │ │  AppFonts    │ │              │            │
│  └──────┬───────┘ └──────────────┘ └──────────────┘ └──────────────┘            │
│         │                                                                       │
│         ▼                                                                       │
│  ┌─────────────────────────┐                                                    │
│  │ platform/windows        │                                                    │
│  │ FileSystemAccessWindows │                                                    │
│  └─────────────────────────┘                                                    │
├─────────────────────────────────────────────────────────────────────────────────┤
│                        Godot Engine (裁剪后)                                     │
│              Scene Tree / Signal / Control / 渲染 / OS 抽象                      │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 3.2 层次说明

| 层次                                       | 职责               | 关键组件                                                                                                                                        |
| ------------------------------------------ | ------------------ | ----------------------------------------------------------------------------------------------------------------------------------------------- |
| **基础 UI** (`app/gui`)                    | UI 框架和布局管理  | LayoutManager, PaneManager, AppTabContainer, MultiSplitContainer, PaneFactory, PaneBase, WelcomePane, AppAbout, VersionButton                   |
| **基础工具及定义** (`app/app_core`)        | 平台抽象和基础类型 | AppPaths, FileSystemAccess, FileInfo                                                                                                            |
| **平台** (`app/app_core/platform/windows`) | Windows 平台实现   | FileSystemAccessWindows                                                                                                                         |
| **应用模块** (`app/app_modules`)           | 具体业务功能实现   | file_management (FilePane, NavigationPane, FileSystemTree, FileContextMenu, AddressBar, FileSystem), text_editing (待实现), workspaces (待实现) |
| **设置** (`app/settings`)                  | 应用配置管理       | AppSettings, SettingsPane                                                                                                                       |
| **主题** (`app/themes`)                    | 外观和主题管理     | AppTheme, AppThemeManager, AppIcons, AppFonts, AppScale, theme_classic, theme_modern                                                            |
| **翻译** (`app/translations`)              | 国际化支持         | AppTranslation, PO 文件                                                                                                                         |

---

## 四、模块设计

`app/` 目录下的代码分为两个层面：

- **应用框架**：`gui/`、`app_core/`、`settings/`、`themes/`、`translations/` 等目录，提供窗格系统、布局管理、平台抽象、设置管理、主题等基础设施，不包含具体业务逻辑。
- **应用模块**（`app_modules/`）：基于应用框架构建的具体业务功能模块。每个子目录是一个独立模块，包含业务逻辑和对应的 GUI 组件。通过继承 `PaneBase` 并注册到 `PaneFactory` 来接入框架。

### 4.1 模块总览

```
app/
├── app_node.cpp/h              # 应用根节点，初始化所有子系统
├── app_string_names.h          # 全局字符串名称定义
├── register_app_types.cpp/h    # 类型注册入口
├── gui/                        # 基础 UI 框架
│   ├── layout_manager.cpp/h    #   布局管理器（拆分/标签页/侧边栏）
│   ├── pane_manager.cpp/h      #   窗格生命周期管理
│   ├── pane_factory.cpp/h      #   窗格注册与创建工厂
│   ├── pane_base.cpp/h         #   窗格基类
│   ├── app_tab_container.cpp/h #   标签页容器
│   ├── multi_split_container.cpp/h # 多拆分容器
│   ├── welcome_pane.cpp/h      #   欢迎页窗格
│   └── app_about.cpp/h         #   关于对话框
├── app_core/                   # 基础工具与平台抽象
│   ├── app_paths.cpp/h         #   应用路径管理
│   ├── io/                     #   I/O 抽象
│   │   └── file_system_access.cpp/h  # 文件系统操作抽象层
│   ├── types/                  #   基础数据类型
│   │   └── file_info.cpp/h     #       文件信息数据结构
│   ├── platform/               #   平台实现
│   │   └── windows/
│   │       └── file_system_access_windows.cpp/h  # Windows 平台实现
│   └── tests/                  #   单元测试
├── app_modules/                # 业务功能模块
│   ├── file_management/        #   文件管理模块
│   ├── text_editing/           #   文本编辑模块
│   ├── workspaces/             #   工作空间模块 (待实现)
│   └── example/                #   示例模块 (开发参考)
├── settings/                   # 设置管理
│   ├── app_settings.cpp/h      #   应用设置数据与逻辑
│   └── gui/settings_pane.cpp/h #   设置面板 UI
├── themes/                     # 主题管理
│   ├── app_theme.cpp/h         #   主题基础定义
│   ├── app_theme_manager.cpp/h #   主题管理器
│   ├── app_icons.cpp/h         #   图标管理
│   ├── app_fonts.cpp/h         #   字体管理
│   ├── app_scale.cpp/h         #   界面缩放
│   ├── theme_classic.cpp/h     #   经典主题
│   └── theme_modern.cpp/h      #   现代主题
└── translations/               # 国际化
    ├── app_translation.cpp/h   #   翻译管理
    └── po/                     #   PO 翻译文件
```

### 4.2 应用框架

#### 4.2.1 根节点（AppNode）

应用的场景树根节点，负责初始化和协调所有子系统。

#### 4.2.2 GUI 框架（gui/）

| 组件                | 职责                                                      |
| ------------------- | --------------------------------------------------------- |
| LayoutManager       | 管理整体布局结构（侧边栏、主区域拆分），处理布局保存/恢复 |
| PaneManager         | 管理窗格的创建、关闭、切换、查找                          |
| PaneFactory         | 注册窗格类型，通过类型标识符创建窗格实例                  |
| PaneBase            | 所有窗格的基类，定义标题、图标、序列化/反序列化接口       |
| AppTabContainer     | 支持拖拽拆分、关闭按钮、新建标签按钮                      |
| MultiSplitContainer | 支持任意层级水平/垂直拆分的容器                           |
| WelcomePane         | 欢迎页，应用启动时或空标签页默认显示                      |
| AppAbout            | 关于对话框，显示版本、许可证等信息                        |

#### 4.2.3 基础设施（app_core/）

| 组件                    | 职责                                                        |
| ----------------------- | ----------------------------------------------------------- |
| AppPaths                | 管理应用数据目录、配置文件路径等                            |
| FileSystemAccess        | 文件系统操作的平台抽象层（读写、遍历）                      |
| FileSystemAccessWindows | Windows 平台下 FileSystemAccess 的具体实现                  |
| FileInfo                | 文件/目录的元信息数据结构（名称、路径、大小、时间、图标等） |

#### 4.2.4 设置（settings/）

AppSettings 以 Godot Resource (tres) 格式存储用户配置。SettingsPane 提供面板式设置界面，作为一个 PaneBase 的子类嵌入标签页系统。

#### 4.2.5 主题（themes/）

内置经典（theme_classic）和现代（theme_modern）两套主题。AppThemeManager 负责主题的加载、切换和通知。AppScale 处理界面缩放适配。

#### 4.2.6 翻译（translations/）

基于 Godot 的翻译系统和 PO 文件实现中/英文切换。AppTranslation 封装翻译加载和语言切换逻辑。

### 4.3 应用模块（app_modules/）

每个应用模块遵循统一结构：

```
module_name/
├── module_logic.cpp/h    # 业务逻辑（不含 UI）
├── gui/                  # UI 组件
│   ├── xxx_pane.cpp/h    #   继承 PaneBase 的窗格类
│   └── ...               #   其他 UI 组件
├── tests/                # 单元测试
└── SCsub                 # 构建脚本
```

#### 4.3.1 file_management（文件管理）

文件管理模块是核心业务模块，提供文件浏览、导航和操作功能。

| 组件            | 职责                                                  |
| --------------- | ----------------------------------------------------- |
| FileSystem      | 文件系统业务逻辑层，封装文件操作，定义通用快捷键      |
| FilePane        | 文件窗格（工具栏 + 地址栏 + 文件列表），继承 PaneBase |
| NavigationPane  | 导航窗格（树状视图），嵌入侧边栏                      |
| FileSystemTree  | 通用文件树/列表控件                                   |
| FileContextMenu | 右键上下文菜单                                        |
| AddressBar      | 地址栏（面包屑/输入模式）                             |

#### 4.3.2 text_editing（文本编辑） - 待实现

计划提供文本编辑及 Markdown 支持。

#### 4.3.3 workspaces（工作空间） - 待实现

计划提供工作空间的保存、切换和管理功能。

---

## 五、数据

### 5.1 数据存储

| 数据     | 存储方式        | 位置                                    | 格式                | 说明                               |
| -------- | --------------- | --------------------------------------- | ------------------- | ---------------------------------- |
| 用户配置 | Resource (tres) | `%APPDATA%/NestPanes/app_settings.tres` | Godot Resource 格式 | 主题、语言、快捷键等配置           |
| 布局数据 | ConfigFile      | `%APPDATA%/NestPanes/app_layouts.cfg`   | INI-like            | 窗口大小位置、拆分结构、标签页状态 |

### 5.2 数据流

```
用户操作 → UI 层 (Pane) → 业务逻辑层 (FileSystem)
                                  ↓
                         FileSystemAccess (抽象层)
                                  ↓
                         平台实现 (Windows API)
                                  ↓
                         处理结果 → 信号 → UI 层更新
```

---

## 六、关键设计决策

| 决策            | 选择                                | 理由                                                        |
| --------------- | ----------------------------------- | ----------------------------------------------------------- |
| 构建方式        | 源码级集成，非 GDExtension          | 可修改引擎内部，裁剪不需要的模块，减小体积                  |
| 窗格系统        | PaneBase + PaneFactory 注册模式     | 统一管理所有可嵌入内容，支持序列化/反序列化，便于扩展       |
| 文件系统操作    | 自定义 FileSystemAccess 抽象层      | Godot 内置文件 API 不满足桌面应用需求（系统图标、回收站等） |
| 配置格式        | Godot Resource (.tres) + ConfigFile | 利用 Godot 内置序列化能力，无需额外依赖                     |
| 禁用 Godot 模块 | 3D、物理、GDScript、网络等          | 桌面应用不需要这些功能，减小二进制体积和构建时间            |
| 主题系统        | 同 Godot Editor Theme               | 移植 Godot Editor Theme 实现                                |
