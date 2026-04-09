# NestPanes

简体中文 | [English](README.md)

一款 Windows 桌面应用，基于[Godot 4.6-stable 源码](https://github.com/godotengine/godot/tree/4.6-stable)。

![screenshot](screenshot.png)

## 功能特性

**核心功能**

- 文件管理
	- 文件浏览
		- 列表排序
		- 历史路径
	- 文件导航
	- 基础文件操作
		- 剪切/复制/粘贴
		- 重命名
		- 删除（回收站）
		- 新建文件/文件夹
		- 拖放移动/复制

**界面布局**

- 多标签页
- 可拆分视图
	- 拖放标签拆分
- 保存恢复

## 规划

- 文件管理
	- 文件搜索
	- 图片预览
- 设置
- 主题

---

## 快捷键

| 操作                   | Windows                 | 范围         |
| ---------------------- | ----------------------- | ------------ |
| 新建标签页             | `Ctrl+T`                | 标签页       |
| 关闭标签页             | `Ctrl+W`                | 标签页       |
| 下一个标签页           | `Ctrl+Tab`              | 标签页       |
| 上一个标签页           | `Ctrl+Shift+Tab`        | 标签页       |
| 复制路径               | `Ctrl+Shift+C`          | 文件管理模块 |
| 在系统资源管理器中显示 | `Ctrl+Alt+R`            | 文件管理模块 |
| 在外部程序中打开       | `Ctrl+Alt+E`            | 文件管理模块 |
| 在终端中打开           | `Ctrl+Alt+T`            | 文件管理模块 |
| 剪切                   | `Ctrl+X`                | 文件管理模块 |
| 复制                   | `Ctrl+C`                | 文件管理模块 |
| 粘贴                   | `Ctrl+V`                | 文件管理模块 |
| 重命名                 | `F2`                    | 文件管理模块 |
| 删除                   | `Delete`                | 文件管理模块 |
| 聚焦路径栏             | `Ctrl+L`                | 文件窗格     |
| 上一个文件夹           | `Alt+Left`, `Backspace` | 文件窗格     |
| 下一个文件夹           | `Alt+Right`             | 文件窗格     |
| 上级文件夹             | `Alt+Up`                | 文件窗格     |
| 刷新                   | `F5`                    | 文件窗格     |
| 切换左侧边栏           | `Ctrl+B`                | 全局         |
| 切换右侧边栏           | `Ctrl+Shift+B`          | 全局         |
| 退出                   | `Ctrl+Q`                | 全局         |

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
