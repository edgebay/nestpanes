# Daily Engine

简体中文 | [English](README.md)

一款 Windows 桌面应用，基于[Godot 4.5-stable 源码](https://github.com/godotengine/godot/tree/4.5-stable)。

TODO: 截图（图片直接放在根目录）：展开导航树，拆分文件窗格左侧一行右侧上下两行

## 功能特性

**核心功能**

- 文件管理
	- 文件浏览
	- 文件导航
	- 基础文件操作

**界面布局**

- 多标签页
- 可拆分视图
	- 拖放标签拆分

## 规划

- 文件管理
	- 状态信息
	- 详细信息
	- 文件搜索
	- 图片预览
- 设置
- 主题

## 快捷键

| 操作       | Windows          |
| ---------- | ---------------- |
| 新建标签页 | `Ctrl+T`         |
| 关闭标签页 | `Ctrl+W`         |
| 下一标签页 | `Ctrl+Tab`       |
| 上一标签页 | `Ctrl+Shift+Tab` |
| 切换侧边栏 | `Ctrl+B`         |
| 退出       | `Ctrl+Q`         |

## 从源码构建

### 环境配置

参考 Godot 引擎开发文档。

### 编译

```
scons
```

### 单元测试

使用`tests`编译选项进行编译：

```
scons tests=yes
```

运行测试：

```
./bin/<binary> --test --test-case-exclude="*[PCKPacker]*"
```
