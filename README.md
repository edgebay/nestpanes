# Daily Engine

[简体中文](README_zh.md) | English

A Windows desktop application based on the [Godot 4.5-stable source code](https://github.com/godotengine/godot/tree/4.5-stable).

## Features

**核心功能**

- 文件管理
	- 文件浏览
	- 基础文件操作

**界面布局**

- 多标签页
- 可拆分视图
	- 拖放标签页拆分

## Roadmap

- 文件管理
	- 状态信息
	- 详细信息
	- 文件搜索
	- 图片预览
- 文本编辑器

---

## Building

```
scons
```

## Unit testing

Compile with the `tests` build option:

```
scons tests=yes
```

Run the tests:

```
./bin/<binary> --test --test-case-exclude="*[PCKPacker]*"
```
