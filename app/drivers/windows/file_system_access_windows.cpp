#include "file_system_access_windows.h"

#include "core/io/image.h"
#include "scene/resources/image_texture.h"

#include <wchar.h>
#include <windows.h>

#include <shellapi.h>

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
