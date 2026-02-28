# NestPanes

[简体中文](README_zh.md) | English

A Windows desktop application based on the [Godot 4.6-stable source code](https://github.com/godotengine/godot/tree/4.6-stable).

![screenshot](screenshot.png)

## Features

**Core Features**

- File Management
	- File Browsing
	- File Navigation
	- Basic File Operations

**UI Layout**

- Multiple Tabs
- Splittable View
	- Drag-and-drop Tab Splitting
- Save & Restore

## Roadmap

- File Management
	- File Browsing Improvements
		- File List: Add support for sorting, multi-selection, and drag-to-move operations
		- Path Bar: Add history path display and navigation
	- File Search
	- Image Preview
- Settings
- Themes

---

## Keyboard Shortcuts

| Action               | Windows          |
| -------------------- | ---------------- |
| New Tab              | `Ctrl+T`         |
| Close Tab            | `Ctrl+W`         |
| Next Tab             | `Ctrl+Tab`       |
| Previous Tab         | `Ctrl+Shift+Tab` |
| Toggle Left Sidebar  | `Ctrl+B`         |
| Toggle Right Sidebar | `Ctrl+Shift+B`   |
| Quit                 | `Ctrl+Q`         |

---

## Building from Source

### Environment Setup

Please refer to the Godot Engine development documentation for setup instructions.

### Building

```
scons
```

### Testing

Build with the `tests` flag:

```
scons tests=yes
```

Run the tests:

```
./bin/<binary> --test --test-case-exclude="*[PCKPacker]*"
```
