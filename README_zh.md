# NestPanes

简体中文 | [English](README.md)

一款 Windows 桌面应用，基于[Godot 4.6-stable 源码](https://github.com/godotengine/godot/tree/4.6-stable)。

![screenshot](screenshot.png)

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
- 保存恢复

## 规划

- 文件管理
	- 文件浏览功能完善
		- 文件列表添加支持排序、多选、拖放移动等
		- 路径栏添加展示历史路径等
	- 文件搜索
	- 图片预览
- 设置
- 主题

---

## 快捷键

| 操作         | Windows          |
| ------------ | ---------------- |
| 新建标签页   | `Ctrl+T`         |
| 关闭标签页   | `Ctrl+W`         |
| 下一个标签页 | `Ctrl+Tab`       |
| 上一个标签页 | `Ctrl+Shift+Tab` |
| 切换左侧边栏 | `Ctrl+B`         |
| 切换右侧边栏 | `Ctrl+Shift+B`   |
| 退出         | `Ctrl+Q`         |

---

## 从源码构建

### 环境配置

参考 Godot 引擎开发文档。

### 编译

```
scons
```

### 测试

使用`tests`编译选项进行编译：

```
scons tests=yes
```

运行测试：

```
./bin/<binary> --test --test-case-exclude="*[PCKPacker]*"
```
