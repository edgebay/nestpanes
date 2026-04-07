#include "file_system_access_windows.h"

#include "core/io/image.h"
#include "scene/resources/image_texture.h"

#include <wchar.h>
#include <windows.h>

#include <shellapi.h>
#include <shlobj.h> // for DROPFILES

// 将 HICON 转换为 Image（RGBA8 格式）
static Ref<Image> _icon_to_image(HICON hIcon, float p_alpha_multiplier = 1) {
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

		if (a != 0 && p_alpha_multiplier < 1) {
			// 调整 alpha 值
			float new_alpha = a * p_alpha_multiplier;
			a = CLAMP((int)new_alpha, 0, 255);
		}

		// 转换为 RGBA8
		data[i + 0] = r;
		data[i + 1] = g;
		data[i + 2] = b;
		data[i + 3] = a;
	}

	// Godot 图像格式：RGBA8
	return memnew(Image(width, height, false, Image::FORMAT_RGBA8, pixels));
}

static uint64_t _get_time_stamp(LPFILETIME time) {
	const uint64_t WINDOWS_TICKS_PER_SECOND = 10000000;
	const uint64_t TICKS_TO_UNIX_EPOCH = 116444736000000000LL;

	uint64_t ret = 0;

	ret = time->dwHighDateTime;
	ret <<= 32;
	ret |= time->dwLowDateTime;

	if (ret >= TICKS_TO_UNIX_EPOCH) {
		return (ret - TICKS_TO_UNIX_EPOCH) / WINDOWS_TICKS_PER_SECOND;
	}

	return 0;
}

struct FileSystemAccessWindowsPrivate {
	HANDLE h; // handle for FindFirstFile.
	WIN32_FIND_DATA f;
	WIN32_FIND_DATAW fu; // Unicode version.

	String path = "";
};

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

Error FileSystemAccessWindows::_list_dir_begin(const String &p_path) {
	ERR_FAIL_COND_V_MSG(p_path == COMPUTER_PATH, ERR_INVALID_PARAMETER, "Use list_drives() to list computer drives.");

	GLOBAL_LOCK_FUNCTION

	_list_dir_end();
	p->h = FindFirstFileExW((LPCWSTR)(String(p_path + "\\*").utf16().get_data()), FindExInfoStandard, &p->fu, FindExSearchNameMatch, nullptr, 0);

	if (p->h == INVALID_HANDLE_VALUE) {
		return ERR_CANT_OPEN;
	}

	p->path = p_path;

	return OK;
}

Error FileSystemAccessWindows::_get_next(FileInfo &r_info) {
	if (p->h == INVALID_HANDLE_VALUE) {
		return FAILED;
	}

	// Skip system files.
	while (p->fu.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) {
		// print_line("skip: ", String::utf16((const char16_t *)(p->fu.cFileName)));
		if (FindNextFileW(p->h, &p->fu) == 0) {
			FindClose(p->h);
			p->h = INVALID_HANDLE_VALUE;
			p->path = "";
			return FAILED;
		}
	}

	String name = String::utf16((const char16_t *)(p->fu.cFileName));

	// if (!(name == "." || name == ".."))
	bool is_dir = (p->fu.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
	bool is_hidden = (p->fu.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN);

	r_info.name = name;
	r_info.path = p->path.path_join(name);

	r_info.icon = _get_icon(r_info.path, is_dir, is_hidden);

	r_info.creation_time = _get_time_stamp(&p->fu.ftCreationTime);
	r_info.modified_time = _get_time_stamp(&p->fu.ftLastWriteTime);

	r_info.hidden = is_hidden;

	if (!is_dir) {
		r_info.type = name.get_extension();
		r_info.size = (static_cast<uint64_t>(p->fu.nFileSizeHigh) << 32) + p->fu.nFileSizeLow;
	} else {
		r_info.type = FOLDER_TYPE; // Won't be a drive.
		r_info.size = 0;
	}

	if (FindNextFileW(p->h, &p->fu) == 0) {
		FindClose(p->h);
		p->h = INVALID_HANDLE_VALUE;
		p->path = "";
	}

	return OK;
}

void FileSystemAccessWindows::_list_dir_end() {
	GLOBAL_LOCK_FUNCTION

	if (p->h != INVALID_HANDLE_VALUE) {
		FindClose(p->h);
		p->h = INVALID_HANDLE_VALUE;
		p->path = "";
	}
}

Ref<Texture2D> FileSystemAccessWindows::_get_icon(const String &p_file_path, bool p_is_dir, bool p_is_hidden) const {
	if (p_file_path == COMPUTER_PATH) {
		return _get_this_pc_icon();
	}

	SHFILEINFO sfi = { 0 };

	DWORD file_attributes = p_is_dir ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;

	// 请求获取图标
	// DWORD res = SHGetFileInfoW(
	// 		(LPCWSTR)(p_file_path.utf16().get_data()),
	DWORD res = SHGetFileInfo(
			(p_file_path.utf8().get_data()),
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
	float alpha_multiplier = 1;
	if (p_is_hidden) {
		alpha_multiplier = 0.6;
	}
	Ref<Image> img = _icon_to_image(sfi.hIcon, alpha_multiplier);

	// 销毁 HICON（必须）
	DestroyIcon(sfi.hIcon);

	if (img.is_null() || img->is_empty()) {
		return Ref<Texture2D>();
	}

	// 创建纹理
	Ref<ImageTexture> texture = ImageTexture::create_from_image(img);

	return texture;
}

// TODO
// 判断是否是本地磁盘（固定磁盘或可移动磁盘）
bool is_local_drive(const String &p_drive_ath) {
	UINT driveType = GetDriveTypeW((LPCWSTR)(p_drive_ath.utf16().get_data()));
	return (driveType == DRIVE_FIXED || driveType == DRIVE_REMOVABLE);
}

Error FileSystemAccessWindows::_get_file_info(const String &p_file_path, FileInfo &r_info) {
	// print_line("get fi: ", p_file_path);
	WIN32_FILE_ATTRIBUTE_DATA fileData;
	if (!GetFileAttributesExW((LPCWSTR)(p_file_path.utf16().get_data()), GetFileExInfoStandard, &fileData)) {
		return FAILED;
	}

	bool is_dir = (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
	bool is_hidden = (fileData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0;

	// TODO: Sync the name case to the actual filesystem name.
	r_info.name = p_file_path.get_file();
	r_info.path = p_file_path;

	r_info.icon = _get_icon(r_info.path, is_dir, is_hidden);

	r_info.creation_time = _get_time_stamp(&fileData.ftCreationTime);
	r_info.modified_time = _get_time_stamp(&fileData.ftLastWriteTime);

	r_info.hidden = is_hidden;

	if (!is_dir) {
		r_info.type = r_info.name.get_extension();
		r_info.size = ((uint64_t)fileData.nFileSizeHigh << 32) | fileData.nFileSizeLow;
	} else {
		r_info.type = FOLDER_TYPE;
		if (p_file_path.length() <= 3) {
			List<FileInfo> drives;
			Error err = _list_drives(drives);
			// print_line("check drives: ", err);
			if (err == OK) {
				for (const FileInfo &drive : drives) {
					if (drive.path == p_file_path) {
						r_info.type = DRIVE_TYPE;
						break;
					}
				}
			}
		}

		r_info.size = 0;
	}
	// print_line("type: ", r_info.type);

	return OK;
}

#if 0 // TODO: handle DRIVE_TYPE
Error FileSystemAccessWindows::_list_file_infos(const String &p_dir, List<FileInfo> &r_subdirs, List<FileInfo> &r_files, FileSortOption p_file_sort) const {
	if (p_dir == COMPUTER_PATH) {
		return _list_drives(r_subdirs);
	}

	if (!dir_exists(p_dir)) {
		return ERR_FILE_BAD_PATH;
	}

	WIN32_FIND_DATAW fu = { 0 };
	HANDLE h = FindFirstFileExW((LPCWSTR)(String(p_dir + "\\*").utf16().get_data()), FindExInfoStandard, &fu, FindExSearchNameMatch, nullptr, 0);
	if (h == INVALID_HANDLE_VALUE) {
		return FAILED;
	}
	while (FindNextFileW(h, &fu)) {
		if (fu.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) {
			continue;
		}

		bool is_dir = (fu.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
		bool is_hidden = (fu.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN);

		String name = String::utf16((const char16_t *)(fu.cFileName));
		if (name == "." || name == "..") {
			continue;
		}

		FileInfo file_info;
		file_info.name = name;
		file_info.path = p_dir.path_join(name);

		file_info.icon = _get_icon(file_info.path, is_dir, is_hidden);

		file_info.creation_time = _get_time_stamp(&fu.ftCreationTime);
		file_info.modified_time = _get_time_stamp(&fu.ftLastWriteTime);

		file_info.hidden = is_hidden;

		if (!is_dir) {
			file_info.type = name.get_extension();
			file_info.size = (static_cast<uint64_t>(fu.nFileSizeHigh) << 32) + fu.nFileSizeLow;

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
#endif

Error FileSystemAccessWindows::_list_drives(List<FileInfo> &r_drives) const {
	const uint64_t MAX_DRIVES = 26;
	DWORD mask = GetLogicalDrives();
	for (int i = 0; i < MAX_DRIVES; i++) {
		if (mask & (1 << i)) { //DRIVE EXISTS
			char drive = 'A' + i;
			String name = String::chr(drive) + ":";

			FileInfo file_info;
			file_info.name = name;
			file_info.path = name;

			file_info.icon = _get_icon(file_info.path, true);

			file_info.creation_time = 0;
			file_info.modified_time = 0; // FileAccess::get_modified_time(file_info.path);

			file_info.hidden = false;

			file_info.type = DRIVE_TYPE;
			file_info.size = 0;

			r_drives.push_back(file_info);
		}
	}

	return OK;
}

bool FileSystemAccessWindows::_canonicalize_path(const String &p_path, String &r_canonicalized) {
	if (!dir_exists(p_path)) {
		return false;
	}

	GLOBAL_LOCK_FUNCTION

	bool result = false;

	// Won't change case of path
	// // DWORD buffsize = p_path.size();
	// DWORD buffsize = GetFullPathNameW((LPCWSTR)(p_path.utf16().get_data()), 0, nullptr, nullptr);
	// Char16String buf;
	// buf.resize_uninitialized(buffsize + 1);
	// DWORD len = GetFullPathNameW((LPCWSTR)(p_path.utf16().get_data()), buffsize + 1, (LPWSTR)buf.ptrw(), nullptr);
	// if (len > 0 && len <= buffsize) {
	// 	result = true;
	// 	String path = String::utf16((const char16_t *)buf.ptr());
	// 	r_canonicalized = path.simplify_path();
	// }

	String name = p_path;
	// "d:" -> "d:\\"
	if (name.length() == 2 && name[1] == ':') {
		name += "\\";
	}

	HANDLE hFile = CreateFileW(
			(LPCWSTR)(name.utf16().get_data()),
			FILE_READ_ATTRIBUTES, // 只需读取属性
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			NULL,
			OPEN_EXISTING,
			FILE_FLAG_BACKUP_SEMANTICS, // 必需：支持目录
			NULL);

	if (hFile == INVALID_HANDLE_VALUE) {
		return false;
	}

	DWORD buffsize = GetFinalPathNameByHandleW(hFile, nullptr, 0, FILE_NAME_NORMALIZED);
	Char16String buf;
	buf.resize_uninitialized(buffsize + 1);
	DWORD len = GetFinalPathNameByHandleW(hFile, (LPWSTR)buf.ptrw(), buffsize + 1, FILE_NAME_NORMALIZED);
	if (len > 0 && len <= buffsize) {
		result = true;
		String path = String::utf16((const char16_t *)buf.ptr());
		if (path.begins_with("\\\\?\\UNC\\")) {
			path = path.substr(8); // Remove "\\?\UNC\" prefix
			path = "\\\\" + path; // Add leading backslash for UNC path
		} else if (path.begins_with("\\\\?\\")) {
			path = path.substr(4); // Remove "\\?\" prefix
		}
		r_canonicalized = path.simplify_path();
	}

	CloseHandle(hFile);

	// print_line("canonicalize: ", result, uint64_t(buffsize), uint64_t(len), p_path, name, r_canonicalized);

	return result;
}

bool FileSystemAccessWindows::_path_exists(const String &p_path) const {
	GLOBAL_LOCK_FUNCTION

	DWORD fileAttr;
	fileAttr = GetFileAttributesW((LPCWSTR)(p_path.utf16().get_data()));
	if (INVALID_FILE_ATTRIBUTES == fileAttr) {
		return false;
	}

	return true;
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
		Char16String tmpfile_utf16 = path.replace("/", "\\").utf16();
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
		Char16String tmpfile_utf16 = path.replace("/", "\\").utf16();
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

bool FileSystemAccessWindows::_paste(const String &p_dir, Vector<String> &r_dest_paths) {
	if (!dir_exists(p_dir)) {
		return false;
	}

	// Retrieve the paste type and the files to be pasted.
	bool is_cut = false;
	Vector<String> files;
	if (!_get_clipboard_paths(files, is_cut)) {
		return false;
	}

	// Paste.
	Error err = _move(!is_cut, p_dir, files, r_dest_paths);

	return err == OK;
}

bool FileSystemAccessWindows::_can_paste() {
	if (!OpenClipboard(nullptr)) {
		return false;
	}

	bool result = IsClipboardFormatAvailable(CF_HDROP);

	CloseClipboard();

	return result;
}

bool FileSystemAccessWindows::_get_clipboard_paths(Vector<String> &r_paths, bool &r_is_cut) {
	bool result = false;

	if (!OpenClipboard(nullptr)) {
		return false;
	}

	if (!IsClipboardFormatAvailable(CF_HDROP)) {
		CloseClipboard();
		return false;
	}

	HDROP hDrop = static_cast<HDROP>(GetClipboardData(CF_HDROP));
	if (!hDrop) {
		CloseClipboard();
		return false;
	}

	UINT fileCount = DragQueryFileW(hDrop, 0xFFFFFFFF, nullptr, 0);
	if (fileCount == 0) {
		CloseClipboard();
		return false;
	}

	DWORD dropEffect = DROPEFFECT_COPY; // Default to copy.
	UINT cfDropEffect = RegisterClipboardFormatW(L"Preferred DropEffect");
	HANDLE hEffect = GetClipboardData(cfDropEffect);
	if (hEffect) {
		DWORD *pEffect = static_cast<DWORD *>(GlobalLock(hEffect));
		if (pEffect) {
			dropEffect = *pEffect;
			GlobalUnlock(hEffect);
		}
	}

	result = true;
	for (UINT i = 0; i < fileCount; ++i) {
		UINT buffsize = DragQueryFileW(hDrop, i, nullptr, 0);
		Char16String buf;
		buf.resize_uninitialized(buffsize + 1);
		if (DragQueryFileW(hDrop, i, (LPWSTR)buf.ptrw(), buffsize + 1) == 0) {
			result = false;
			break;
		}
		String path = String::utf16((const char16_t *)buf.ptr());
		r_paths.push_back(path.simplify_path());
	}
	CloseClipboard();

	if (result) {
		r_is_cut = (dropEffect & DROPEFFECT_MOVE);
	}

	return result;
}

Error FileSystemAccessWindows::_move(bool p_is_copy, const String &p_to_dir, const Vector<String> &p_from_paths, Vector<String> &r_dest_paths) {
	if (!dir_exists(p_to_dir)) {
		return FAILED;
	}

	HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	if (FAILED(hr)) {
		return FAILED;
	}

	Error err = FAILED;
	IFileOperation *pfo = nullptr;
	IShellItem *pTargetFolder = nullptr;
	Vector<IShellItem *> sourceItems;

	// 1. 创建 IFileOperation
	hr = CoCreateInstance(CLSID_FileOperation, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&pfo));

	if (SUCCEEDED(hr)) {
		// 2. 设置不显示 UI（设为 TRUE 可显示进度、冲突对话框）
		// hr = pfo->SetOperationFlags(FOF_NO_UI); // 默认是 FOF_NO_UI = 0，即显示 UI
		// 若希望静默操作，取消注释下一行：
		// hr = pfo->SetOperationFlags(FOF_NO_UI | FOF_NOCONFIRMATION | FOF_SILENT);
	}

	String dir = p_to_dir.replace("/", "\\");
	if (SUCCEEDED(hr)) {
		// 3. 获取目标文件夹 IShellItem
		hr = SHCreateItemFromParsingName((PCWSTR)(dir.utf16().get_data()), nullptr, IID_PPV_ARGS(&pTargetFolder));
	}

	if (SUCCEEDED(hr)) {
		// 遍历所有文件并执行操作
#if 1 // TODO
		hr = E_UNEXPECTED;
		for (const String &file : p_from_paths) {
			String src_path = file.replace_char('/', '\\');
			String filename = src_path.get_file();
			String dest_path = dir + "\\" + filename; // Note: use '\\'

			Char16String src_file_utf16 = src_path.utf16();
			IShellItem *pItem = nullptr;
			hr = SHCreateItemFromParsingName((PCWSTR)src_file_utf16.ptr(), nullptr, IID_PPV_ARGS(&pItem));
			if (SUCCEEDED(hr) && pItem) {
				sourceItems.push_back(pItem);

				if (!p_is_copy) {
					hr = pfo->MoveItem(pItem, pTargetFolder, nullptr, nullptr);
				} else {
					// print_line("paste: ", dest_path, src_path);
					if (dest_path == src_path) {
						bool is_dir = FileSystemAccess::dir_exists(src_path);
						String name = filename;
						String path = dest_path;

						int i = 1;
						while (FileSystemAccess::path_exists(path)) {
							if (is_dir || filename.get_extension().is_empty()) {
								name = vformat("%s (%d)", filename, i);
							} else {
								name = vformat("%s (%d).%s", filename.get_basename(), i, filename.get_extension());
							}

							path = dir + "\\" + name; // Note: use '\\'
							i++;
						}
						filename = name;
						dest_path = path;
					}
					hr = pfo->CopyItem(pItem, pTargetFolder, (LPCWSTR)(filename.utf16().get_data()), nullptr);
				}

				r_dest_paths.push_back(dest_path.simplify_path());
			}

			if (FAILED(hr)) {
				r_dest_paths.clear();
				break;
			}
		}

		if (SUCCEEDED(hr)) {
			// 7. 执行操作（会弹出标准 Windows 进度/冲突对话框）
			hr = pfo->PerformOperations();
			if (SUCCEEDED(hr)) {
				// 8. 检查是否成功完成
				BOOL anyOperationsAborted = FALSE;
				hr = pfo->GetAnyOperationsAborted(&anyOperationsAborted);
				if (SUCCEEDED(hr) && !anyOperationsAborted) {
					err = OK;
				}
			}
		}
#else
		for (const String &file : p_from_paths) {
			String src_path = file.replace_char('/', '\\');
			String filename = src_path.get_file();
			String dest_path = dir + "\\" + filename;
			Char16String src_file_utf16 = src_path.utf16();

			BOOL result = FALSE;
			if (!p_is_copy) {
				result = MoveFileW((LPCWSTR)src_file_utf16.get_data(), (LPCWSTR)(dest_path.utf16().get_data()));
			} else {
				result = CopyFileW((LPCWSTR)src_file_utf16.get_data(), (LPCWSTR)(dest_path.utf16().get_data()), FALSE); // FALSE = 覆盖
			}

			if (!result) {
				DWORD last_err = GetLastError();
				// 可选择继续或中止
			} else {
				err = OK;
			}
		}
#endif
	}

	// 清理资源
	for (IShellItem *item : sourceItems) {
		item->Release();
	}
	if (pTargetFolder) {
		pTargetFolder->Release();
	}
	if (pfo) {
		pfo->Release();
	}
	CoUninitialize();

	return err;
}

Error FileSystemAccessWindows::_create_file(const String &p_dir, const String &p_filename) {
	String file = p_dir.path_join(p_filename);
	if (file_exists(file)) {
		return OK;
	}

	HANDLE hFile = CreateFileW(
			(LPCWSTR)(file.utf16().get_data()),
			GENERIC_WRITE,
			0, // 不共享
			nullptr, // 默认安全属性
			CREATE_ALWAYS, // 总是创建新文件（覆盖现有文件）
			FILE_ATTRIBUTE_NORMAL,
			nullptr);

	if (hFile == INVALID_HANDLE_VALUE) {
		return FAILED;
	}

	CloseHandle(hFile);
	return OK;
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
	p = memnew(FileSystemAccessWindowsPrivate);
	p->h = INVALID_HANDLE_VALUE;

	set_system_icon(SNAME(COMPUTER_PATH), _get_this_pc_icon());
	_update_drives_icon(); // TODO: Handle drive change
	// // Folder CLSID - ::{59031a47-3f73-442d-9ca1-6c09bfe28fef}
	// set_system_icon(SNAME("Folder"), _get_icon("::{59031a47-3f73-442d-9ca1-6c09bfe28fef}", true));
	set_system_icon(SNAME("Folder"), _get_icon("Folder", true));
}

FileSystemAccessWindows::~FileSystemAccessWindows() {
	_list_dir_end();

	memdelete(p);
}
