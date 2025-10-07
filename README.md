# Daily Engine

包含文件管理及文本编辑功能的 Windows 桌面应用。

基于[Godot 源码 4.5-stable](https://github.com/godotengine/godot/tree/4.5-stable)。

---

A Windows desktop application integrating file management and text editing functions.

Based on [Godot source 4.5-stable](https://github.com/godotengine/godot/tree/4.5-stable).

## 编译 Building

```
scons
```

## 单元测试 Unit testing

使用`tests`编译选项进行编译：

Compile with the `tests` build option:

```
scons tests=yes
```

运行测试：

Run the tests:

```
./bin/<binary> --test --test-case-exclude="*[PCKPacker]*"
```
