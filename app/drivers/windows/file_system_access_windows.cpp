#include "file_system_access_windows.h"

#include "core/io/image.h"
#include "scene/resources/image_texture.h"

#include <wchar.h>
#include <windows.h>

#include <shellapi.h>
#include <shlobj.h> // for DROPFILES

// 将 HICON 转换为 Image（RGBA8 格式）
Ref<Image> _icon_to_image(HICON hIcon) {
	if (!hIcon) {
		return Ref<Image>();
	}

	ICONINFO iconInfo = { 0 };
	if (!GetIconInfo(hIcon, &iconInfo)) {
		return Ref<Image>();
	}

	// 获取图标大小（通常为 32x32，但可变）
	BITMAP bmp = { 0 };
	if (!GetObject(iconInfo.hbmColor, sizeof(BITMAP), &bmp)) {
		DeleteObject(iconInfo.hbmColor);
		DeleteObject(iconInfo.hbmMask);
		return Ref<Image>();
	}

	int width = bmp.bmWidth;
	int height = bmp.bmHeight;

	// 创建设备上下文（DC）
	HDC hdcScreen = GetDC(nullptr);
	HDC hdcMem = CreateCompatibleDC(hdcScreen);
	HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, width, height);
	HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);

	// 绘制图标到位图
	DrawIconEx(hdcMem, 0, 0, hIcon, width, height, 0, nullptr, DI_NORMAL);

	// 锁定位图内存
	BITMAPINFO bmi = { 0 };
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = width;
	bmi.bmiHeader.biHeight = -height; // Top-down DIB
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	PackedByteArray pixels;
	pixels.resize(width * height * 4);
	uint8_t *data = pixels.ptrw();

	GetDIBits(hdcMem, hBitmap, 0, height, data, &bmi, DIB_RGB_COLORS);

	// 清理 GDI 资源
	SelectObject(hdcMem, hOldBitmap);
	DeleteObject(hBitmap);
	DeleteDC(hdcMem);
	ReleaseDC(nullptr, hdcScreen);
	DeleteObject(iconInfo.hbmColor);
	if (iconInfo.hbmMask) {
		DeleteObject(iconInfo.hbmMask);
	}

	// BGRA 格式转为 RGBA8 格式
	for (int i = 0; i < width * height * 4; i += 4) {
		uint8_t b = data[i + 0];
		uint8_t g = data[i + 1];
		uint8_t r = data[i + 2];
		uint8_t a = data[i + 3];

		// 转换为 RGBA8
		data[i + 0] = r;
		data[i + 1] = g;
		data[i + 2] = b;
		data[i + 3] = a;
	}

	// Godot 图像格式：RGBA8
	return memnew(Image(width, height, false, Image::FORMAT_RGBA8, pixels));
}

void FileSystemAccessWindows::_update_drives_icon() {
	DWORD mask = GetLogicalDrives();
	if (drives_mask == mask) {
		return;
	}

	const uint64_t MAX_DRIVES = 26;
	for (int i = 0; i < MAX_DRIVES; i++) {
		char drive = 'A' + i;
		String path = String::chr(drive) + ":";
		if (mask & (1 << i)) {
			set_system_icon(path, _get_icon(path, true));
		} else if (has_system_icon(path)) {
			clear_system_icon(path);
		}
	}
	drives_mask = mask;
}

Ref<Texture2D> FileSystemAccessWindows::_get_this_pc_icon() const {
	SHSTOCKICONINFO sii = { 0 };
	sii.cbSize = sizeof(sii);

	// 获取“此电脑”图标
	// HRESULT hr = SHGetStockIconInfo(SIID_DESKTOPPC, SHGSI_ICON | SHGSI_LARGEICON, &sii);
	HRESULT hr = SHGetStockIconInfo(SIID_DESKTOPPC, SHGSI_ICON | SHGSI_SMALLICON, &sii);
	if (FAILED(hr) || sii.hIcon == nullptr) {
		return Ref<Texture2D>();
	}

	Ref<Image> img = _icon_to_image(sii.hIcon);
	DestroyIcon(sii.hIcon);

	if (img.is_null() || img->is_empty()) {
		return Ref<Texture2D>();
	}

	return ImageTexture::create_from_image(img);
}

Ref<Texture2D> FileSystemAccessWindows::_get_icon(const String &p_file_path, bool p_is_dir) const {
	if (p_file_path == COMPUTER_PATH) {
		return _get_this_pc_icon();
	}

	SHFILEINFO sfi = { 0 };

	DWORD file_attributes = p_is_dir ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;

	// 请求获取图标（大图标）
	DWORD res = SHGetFileInfo(
			p_file_path.utf8().get_data(),
			file_attributes,
			&sfi,
			sizeof(sfi),
			// TODO: icon size
			// SHGFI_ICON | SHGFI_LARGEICON | SHGFI_USEFILEATTRIBUTES
			SHGFI_ICON |
					SHGFI_SMALLICON |
					SHGFI_TYPENAME |
					SHGFI_USEFILEATTRIBUTES);

	if (res == 0 || sfi.hIcon == nullptr) {
		return Ref<Texture2D>();
	}

	// 转换为 Image
	Ref<Image> img = _icon_to_image(sfi.hIcon);

	// 销毁 HICON（必须）
	DestroyIcon(sfi.hIcon);

	if (img.is_null() || img->is_empty()) {
		return Ref<Texture2D>();
	}

	// 创建纹理
	Ref<ImageTexture> texture = ImageTexture::create_from_image(img);

	return texture;
}

Error FileSystemAccessWindows::_list_file_infos(const String &p_dir, List<FileInfo> &r_subdirs, List<FileInfo> &r_files, FileSortOption p_file_sort) const {
	if (p_dir == COMPUTER_PATH) {
		return _list_drives(r_subdirs);
	}

	if (!_dir_exists(p_dir)) {
		return ERR_FILE_BAD_PATH;
	}

	WIN32_FIND_DATAW fu = { 0 };
	HANDLE h = FindFirstFileExW((LPCWSTR)(String(p_dir + "\\*").utf16().get_data()), FindExInfoStandard, &fu, FindExSearchNameMatch, nullptr, 0);
	if (h == INVALID_HANDLE_VALUE) {
		return FAILED;
	}
	while (FindNextFileW(h, &fu)) {
		bool is_dir = (fu.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);

		String name = String::utf16((const char16_t *)(fu.cFileName));
		if (name == "." || name == "..") {
			continue;
		}

		FileInfo file_info;
		file_info.name = name;
		file_info.path = p_dir.path_join(name);

		file_info.icon = _get_icon(file_info.path, is_dir);

		// FILETIME ft_create, ft_write;
		// bool status = GetFileTime(h, &ft_create, nullptr, &ft_write);
		uint64_t last_write_time = 0;
		last_write_time = fu.ftLastWriteTime.dwHighDateTime;
		last_write_time <<= 32;
		last_write_time |= fu.ftLastWriteTime.dwLowDateTime;
		const uint64_t WINDOWS_TICKS_PER_SECOND = 10000000;
		const uint64_t TICKS_TO_UNIX_EPOCH = 116444736000000000LL;
		if (last_write_time >= TICKS_TO_UNIX_EPOCH) {
			last_write_time = (last_write_time - TICKS_TO_UNIX_EPOCH) / WINDOWS_TICKS_PER_SECOND;
		}
		file_info.modified_time = last_write_time; // FileAccess::get_modified_time(file_info.path);

		file_info.is_hidden = (fu.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN);

		if (!is_dir) {
			file_info.type = name.get_extension();
			file_info.size = (static_cast<int64_t>(fu.nFileSizeHigh) << 32) + fu.nFileSizeLow;

			r_files.push_back(file_info);
		} else {
			file_info.type = FOLDER_TYPE;
			file_info.size = 0;

			r_subdirs.push_back(file_info);
		}
	}
	FindClose(h);

	if (r_subdirs.size() > 1) {
		sort_file_info_list(r_subdirs, p_file_sort);
	}
	if (r_files.size() > 1) {
		sort_file_info_list(r_files, p_file_sort);
	}

	return OK;
}

Error FileSystemAccessWindows::_list_drives(List<FileInfo> &r_drives) const {
	const uint64_t MAX_DRIVES = 26;
	DWORD mask = GetLogicalDrives();
	for (int i = 0; i < MAX_DRIVES; i++) {
		if (mask & (1 << i)) { //DRIVE EXISTS
			char drive = 'A' + i;
			String name = String::chr(drive) + ":";
			// print_line("drive name: " + name);

			FileInfo file_info;
			file_info.name = name;
			file_info.path = name;

			file_info.icon = _get_icon(file_info.path, true);

			file_info.modified_time = 0; // FileAccess::get_modified_time(file_info.path);

			file_info.is_hidden = false;

			file_info.type = FOLDER_TYPE;
			file_info.size = 0;

			// _fill_file_info(file_info, true);

			r_drives.push_back(file_info);
		}
	}

	return OK;
}

Error FileSystemAccessWindows::_make_dir(const String &p_dir) {
	GLOBAL_LOCK_FUNCTION

	if (FileSystemAccessWindows::is_path_invalid(p_dir)) {
#ifdef DEBUG_ENABLED
		WARN_PRINT("The path :" + p_dir + " is a reserved Windows system pipe, so it can't be used for creating directories.");
#endif
		return ERR_INVALID_PARAMETER;
	}

	bool success;
	int err;

	success = CreateDirectoryW((LPCWSTR)(p_dir.utf16().get_data()), nullptr);
	err = GetLastError();

	if (success) {
		return OK;
	}

	if (err == ERROR_ALREADY_EXISTS || err == ERROR_ACCESS_DENIED) {
		return ERR_ALREADY_EXISTS;
	}

	return ERR_CANT_CREATE;
}

bool FileSystemAccessWindows::_file_exists(const String &p_file) const {
	GLOBAL_LOCK_FUNCTION

	DWORD fileAttr;
	fileAttr = GetFileAttributesW((LPCWSTR)(p_file.utf16().get_data()));
	if (INVALID_FILE_ATTRIBUTES == fileAttr) {
		return false;
	}

	return !(fileAttr & FILE_ATTRIBUTE_DIRECTORY);
}

bool FileSystemAccessWindows::_dir_exists(const String &p_dir) const {
	GLOBAL_LOCK_FUNCTION

	DWORD fileAttr;
	fileAttr = GetFileAttributesW((LPCWSTR)(p_dir.utf16().get_data()));
	if (INVALID_FILE_ATTRIBUTES == fileAttr) {
		return false;
	}
	return (fileAttr & FILE_ATTRIBUTE_DIRECTORY);
}

bool FileSystemAccessWindows::_open_in_terminal(const String &p_path) {
	return false;
}

bool FileSystemAccessWindows::_cut(const Vector<String> &p_files) {
	if (p_files.is_empty()) {
		return false;
	}

	Vector<Char16String> files;

	// 打开剪切板
	if (!OpenClipboard(nullptr)) {
		return false;
	}

	// 清空剪切板
	EmptyClipboard();

	// ========== 1. 设置 CF_HDROP ==========

	// 计算 DROPFILES 结构 + 所有文件路径所需内存大小
	size_t totalSize = sizeof(DROPFILES);
	for (const auto &path : p_files) {
		// print_line("cut: " + path);

		Char16String tmpfile_utf16 = path.utf16();
		files.push_back(tmpfile_utf16);

		totalSize += (tmpfile_utf16.size() + 1) * sizeof(wchar_t); // 包括 null 分隔符
	}
	totalSize += sizeof(wchar_t); // 结尾双 null（实际只需一个额外 null，但安全起见）

	// 分配全局内存（可移动，GMEM_MOVEABLE）
	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, totalSize);
	if (!hMem) {
		CloseClipboard();
		return false;
	}

	// 锁定内存
	DROPFILES *pDropFiles = static_cast<DROPFILES *>(GlobalLock(hMem));
	if (!pDropFiles) {
		GlobalFree(hMem);
		CloseClipboard();
		return false;
	}

	// 初始化 DROPFILES
	ZeroMemory(pDropFiles, sizeof(DROPFILES));
	pDropFiles->pFiles = sizeof(DROPFILES); // 偏移量：文件路径起始位置
	pDropFiles->fWide = TRUE; // 使用宽字符（Unicode）

	// 在 DROPFILES 后面写入文件路径
	wchar_t *pFileList = reinterpret_cast<wchar_t *>(reinterpret_cast<BYTE *>(pDropFiles) + sizeof(DROPFILES));
	wchar_t *pCurrent = pFileList;

	for (const auto &path : files) {
		int size = path.size(); // len = size - 1
		memcpy(pCurrent, path.ptr(), size * sizeof(wchar_t));

		// wcscpy_s(pCurrent, path.size(), (LPCWSTR)path.get_data());
		// pCurrent += path.size(); // 移动指针，保留 null 分隔
		pCurrent += size;
	}

	// 结尾再加一个 null（表示列表结束）
	*pCurrent = L'\0';

	// 解锁内存
	GlobalUnlock(hMem);

	// 设置剪切板数据
	if (!SetClipboardData(CF_HDROP, hMem)) {
		GlobalFree(hMem);
		CloseClipboard();
		return false;
	}

	// ========== 2. 设置 "Preferred DropEffect" ==========
	// 注册格式（通常已预定义，但安全起见）
	UINT cfDropEffect = RegisterClipboardFormatW(L"Preferred DropEffect");

	HGLOBAL hEffect = GlobalAlloc(GMEM_MOVEABLE, sizeof(DWORD));
	if (!hEffect) {
		CloseClipboard();
		return false;
	}

	DWORD *pEffect = static_cast<DWORD *>(GlobalLock(hEffect));
	if (!pEffect) {
		GlobalFree(hEffect);
		CloseClipboard();
		return false;
	}

	*pEffect = DROPEFFECT_MOVE; // 关键：表示“剪切”， DROPEFFECT_COPY;  // 复制
	GlobalUnlock(hEffect);

	if (!SetClipboardData(cfDropEffect, hEffect)) {
		GlobalFree(hEffect);
		CloseClipboard();
		return false;
	}

	// 关闭剪切板（不要手动释放 hMem，系统会自动管理）
	CloseClipboard();
	return true;
}

bool FileSystemAccessWindows::_copy(const Vector<String> &p_files) {
	if (p_files.is_empty()) {
		return false;
	}

	Vector<Char16String> files;

	// 打开剪切板
	if (!OpenClipboard(nullptr)) {
		return false;
	}

	// 清空剪切板
	EmptyClipboard();

	// 计算 DROPFILES 结构 + 所有文件路径所需内存大小
	size_t totalSize = sizeof(DROPFILES);
	for (const auto &path : p_files) {
		// print_line("copy: " + path);

		Char16String tmpfile_utf16 = path.utf16();
		files.push_back(tmpfile_utf16);

		totalSize += (tmpfile_utf16.size() + 1) * sizeof(wchar_t); // 包括 null 分隔符
	}
	totalSize += sizeof(wchar_t); // 结尾双 null（实际只需一个额外 null，但安全起见）

	// 分配全局内存（可移动，GMEM_MOVEABLE）
	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, totalSize);
	if (!hMem) {
		CloseClipboard();
		return false;
	}

	// 锁定内存
	DROPFILES *pDropFiles = static_cast<DROPFILES *>(GlobalLock(hMem));
	if (!pDropFiles) {
		GlobalFree(hMem);
		CloseClipboard();
		return false;
	}

	// 初始化 DROPFILES
	ZeroMemory(pDropFiles, sizeof(DROPFILES));
	pDropFiles->pFiles = sizeof(DROPFILES); // 偏移量：文件路径起始位置
	pDropFiles->fWide = TRUE; // 使用宽字符（Unicode）

	// 在 DROPFILES 后面写入文件路径
	wchar_t *pFileList = reinterpret_cast<wchar_t *>(reinterpret_cast<BYTE *>(pDropFiles) + sizeof(DROPFILES));
	wchar_t *pCurrent = pFileList;

	for (const auto &path : files) {
		int size = path.size(); // len = size - 1
		memcpy(pCurrent, path.ptr(), size * sizeof(wchar_t));

		// wcscpy_s(pCurrent, path.size(), (LPCWSTR)path.get_data());
		// pCurrent += path.size(); // 移动指针，保留 null 分隔
		pCurrent += size;
	}

	// 结尾再加一个 null（表示列表结束）
	*pCurrent = L'\0';

	// 解锁内存
	GlobalUnlock(hMem);

	// 设置剪切板数据
	if (!SetClipboardData(CF_HDROP, hMem)) {
		GlobalFree(hMem);
		CloseClipboard();
		return false;
	}

	// 关闭剪切板（不要手动释放 hMem，系统会自动管理）
	CloseClipboard();
	return true;
}

bool FileSystemAccessWindows::_paste(const String &p_dir) {
	bool success = false;

	// 确保目标目录存在
	if (!_dir_exists(p_dir)) {
		return false;
	}

	// 打开剪切板
	if (!OpenClipboard(nullptr)) {
		return false;
	}

	// 检查是否有 CF_HDROP 数据
	if (!IsClipboardFormatAvailable(CF_HDROP)) {
		return false;
	}

	HDROP hDrop = static_cast<HDROP>(GetClipboardData(CF_HDROP));
	if (!hDrop) {
		CloseClipboard();
		return false;
	}

	// 获取文件数量
	UINT fileCount = DragQueryFileW(hDrop, 0xFFFFFFFF, nullptr, 0);
	if (fileCount == 0) {
		CloseClipboard();
		return false;
	}

	// 获取操作类型（复制 or 剪切）
	DWORD dropEffect = DROPEFFECT_COPY; // 默认复制
	UINT cfDropEffect = RegisterClipboardFormatW(L"Preferred DropEffect");
	HANDLE hEffect = GetClipboardData(cfDropEffect);
	if (hEffect) {
		DWORD *pEffect = static_cast<DWORD *>(GlobalLock(hEffect));
		if (pEffect) {
			dropEffect = *pEffect;
			GlobalUnlock(hEffect);
		}
	}

	// 遍历所有文件并执行操作
	for (UINT i = 0; i < fileCount; ++i) {
		// 获取源文件路径
		UINT pathLen = DragQueryFileW(hDrop, i, nullptr, 0);
		Char16String src_file_utf16;
		src_file_utf16.resize_uninitialized(pathLen + 1);
		DragQueryFileW(hDrop, i, (LPWSTR)src_file_utf16.ptr(), pathLen + 1);

		// 构造目标路径（保留原文件名）
		String src_path = String::utf16(src_file_utf16.get_data(), src_file_utf16.length());
		String filename = src_path.get_file();
		String dest_path = p_dir + "\\" + filename;

		// print_line("paste: " + src_path + "\nto: " + dest_path);

		// 执行操作
		BOOL result = FALSE;
		if (dropEffect & DROPEFFECT_MOVE) {
			// 剪切 = 移动
			result = MoveFileW((LPCWSTR)src_file_utf16.get_data(), (LPCWSTR)(dest_path.utf16().get_data()));
		} else {
			// 复制
			result = CopyFileW((LPCWSTR)src_file_utf16.get_data(), (LPCWSTR)(dest_path.utf16().get_data()), FALSE); // FALSE = 覆盖
		}

		if (!result) {
			DWORD err = GetLastError();
			// 可选择继续或中止
		} else {
			success = true;
		}
	}

	CloseClipboard();
	return success;
}

Error FileSystemAccessWindows::_rename(String p_path, String p_new_path) {
	String path = p_path;
	String new_path = p_new_path;

	// If we're only changing file name case we need to do a little juggling
	if (path.to_lower() == new_path.to_lower()) {
		if (_dir_exists(path)) {
			// The path is a dir; just rename
			return MoveFileW((LPCWSTR)(path.utf16().get_data()), (LPCWSTR)(new_path.utf16().get_data())) != 0 ? OK : FAILED;
		}
		// The path is a file; juggle
		// Note: do not use GetTempFileNameW, it's not long path aware!
		Char16String tmpfile_utf16;
		uint64_t id = OS::get_singleton()->get_ticks_usec();
		while (true) {
			tmpfile_utf16 = (path + itos(id++) + ".tmp").utf16();
			HANDLE handle = CreateFileW((LPCWSTR)tmpfile_utf16.get_data(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (handle != INVALID_HANDLE_VALUE) {
				CloseHandle(handle);
				break;
			}
			if (GetLastError() != ERROR_FILE_EXISTS && GetLastError() != ERROR_SHARING_VIOLATION) {
				return FAILED;
			}
		}

		if (!::ReplaceFileW((LPCWSTR)tmpfile_utf16.get_data(), (LPCWSTR)(path.utf16().get_data()), nullptr, 0, nullptr, nullptr)) {
			DeleteFileW((LPCWSTR)tmpfile_utf16.get_data());
			return FAILED;
		}

		return MoveFileW((LPCWSTR)tmpfile_utf16.get_data(), (LPCWSTR)(new_path.utf16().get_data())) != 0 ? OK : FAILED;

	} else {
		// TODO: confirmation dialog
		if (file_exists(new_path)) {
			if (remove(new_path) != OK) {
				return FAILED;
			}
		}

		return MoveFileW((LPCWSTR)(path.utf16().get_data()), (LPCWSTR)(new_path.utf16().get_data())) != 0 ? OK : FAILED;
	}
}

Error FileSystemAccessWindows::_remove(String p_path) {
	String path = p_path;
	const Char16String &path_utf16 = path.utf16();

	DWORD fileAttr;

	fileAttr = GetFileAttributesW((LPCWSTR)(path_utf16.get_data()));
	if (INVALID_FILE_ATTRIBUTES == fileAttr) {
		return FAILED;
	}
	if ((fileAttr & FILE_ATTRIBUTE_DIRECTORY)) {
		return RemoveDirectoryW((LPCWSTR)(path_utf16.get_data())) != 0 ? OK : FAILED;
	} else {
		return DeleteFileW((LPCWSTR)(path_utf16.get_data())) != 0 ? OK : FAILED;
	}
}

bool FileSystemAccessWindows::_new_file(const String &p_dir, const String &p_filename) {
	return false;
}

bool FileSystemAccessWindows::is_path_invalid(const String &p_path) {
	// Check for invalid operating system file.
	String fname = p_path.get_file().to_lower();

	int dot = fname.find_char('.');
	if (dot != -1) {
		fname = fname.substr(0, dot);
	}
	return invalid_files.has(fname);
}

Error FileSystemAccessWindows::change_path(const String &p_dir) {
	Error err = FAILED;
	// if (p_dir == COMPUTER_PATH) {
	// 	err = OK;
	// 	path = p_dir;
	// }
	// else {
	// 	err = dir_access->change_dir(p_dir);
	// 	if (err == OK) {
	// 		path = dir_access->get_current_dir();
	// 	}
	// }
	current_path = p_dir;
	err = OK;

	return err;
}

String FileSystemAccessWindows::get_current_path() const {
	return current_path;
}

HashSet<String> FileSystemAccessWindows::invalid_files;

void FileSystemAccessWindows::initialize() {
	static const char *reserved_files[]{
		"con", "prn", "aux", "nul", "com0", "com1", "com2", "com3", "com4", "com5", "com6", "com7", "com8", "com9", "lpt0", "lpt1", "lpt2", "lpt3", "lpt4", "lpt5", "lpt6", "lpt7", "lpt8", "lpt9", nullptr
	};
	int reserved_file_index = 0;
	while (reserved_files[reserved_file_index] != nullptr) {
		invalid_files.insert(reserved_files[reserved_file_index]);
		reserved_file_index++;
	}

	_setmaxstdio(8192);
	print_verbose(vformat("Maximum number of file handles: %d", _getmaxstdio()));
}

void FileSystemAccessWindows::finalize() {
	invalid_files.clear();
}

FileSystemAccessWindows::FileSystemAccessWindows() {
	set_system_icon(SNAME(COMPUTER_PATH), _get_this_pc_icon());
	_update_drives_icon(); // TODO: Handle drive change
	// // Folder CLSID - ::{59031a47-3f73-442d-9ca1-6c09bfe28fef}
	// set_system_icon(SNAME("Folder"), _get_icon("::{59031a47-3f73-442d-9ca1-6c09bfe28fef}", true));
	set_system_icon(SNAME("Folder"), _get_icon("Folder", true));
}

FileSystemAccessWindows::~FileSystemAccessWindows() {
}
