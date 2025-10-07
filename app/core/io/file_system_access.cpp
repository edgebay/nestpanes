#include "file_system_access.h"

HashMap<StringName, Ref<Texture2D>> FileSystemAccess::icon_map;

FileSystemAccess::CreateFunc FileSystemAccess::create_func = nullptr;

Ref<FileSystemAccess> FileSystemAccess::singleton = nullptr;

FileSystemAccess *FileSystemAccess::get_singleton() {
	return singleton.ptr();
}

void FileSystemAccess::create() {
	if (singleton.ptr()) {
		ERR_PRINT("Can't recreate FileSystemAccess as it already exists.");
		return;
	}

	if (create_func) {
		singleton = create_func();
	} else {
		ERR_PRINT("Can't create FileSystemAccess as didn't call make_default.");
	}
}

void FileSystemAccess::destroy() {
	if (!singleton.ptr()) {
		return;
	}
	singleton = Ref<FileSystemAccess>();
}

Ref<Texture2D> FileSystemAccess::get_icon(const String &p_file_path, bool p_is_dir) {
	ERR_FAIL_NULL_V_MSG(FileSystemAccess::get_singleton(), Ref<Texture2D>(), "FileSystemAccess not instantiated yet.");
	return FileSystemAccess::get_singleton()->_get_icon(p_file_path, p_is_dir);
}

Error FileSystemAccess::list_file_infos(const String &p_dir, List<FileInfo> &r_subdirs, List<FileInfo> &r_files, FileSortOption p_file_sort) {
	ERR_FAIL_NULL_V_MSG(FileSystemAccess::get_singleton(), FAILED, "FileSystemAccess not instantiated yet.");
	return FileSystemAccess::get_singleton()->_list_file_infos(p_dir, r_subdirs, r_files, p_file_sort);
}

Error FileSystemAccess::list_drives(List<FileInfo> &r_drives) {
	ERR_FAIL_NULL_V_MSG(FileSystemAccess::get_singleton(), FAILED, "FileSystemAccess not instantiated yet.");
	return FileSystemAccess::get_singleton()->_list_drives(r_drives);
}

bool FileSystemAccess::file_exists(const String &p_file) {
	ERR_FAIL_NULL_V_MSG(FileSystemAccess::get_singleton(), false, "FileSystemAccess not instantiated yet.");
	return FileSystemAccess::get_singleton()->_file_exists(p_file);
}

bool FileSystemAccess::dir_exists(const String &p_dir) {
	ERR_FAIL_NULL_V_MSG(FileSystemAccess::get_singleton(), false, "FileSystemAccess not instantiated yet.");
	return FileSystemAccess::get_singleton()->_dir_exists(p_dir);
}

void FileSystemAccess::set_system_icon(const StringName &p_name, const Ref<Texture2D> &p_icon) {
	ERR_FAIL_COND_MSG(!p_icon.is_valid(), vformat("Invalid icon, name: '%s'", p_name));

	icon_map[p_name] = p_icon;
}

Ref<Texture2D> FileSystemAccess::get_system_icon(const StringName &p_name) {
	if (icon_map.has(p_name) && icon_map[p_name].is_valid()) {
		return icon_map[p_name];
	}
	return Ref<Texture2D>();
}

bool FileSystemAccess::has_system_icon(const StringName &p_name) {
	return icon_map.has(p_name) && icon_map[p_name].is_valid();
}

void FileSystemAccess::clear_system_icon(const StringName &p_name) {
	ERR_FAIL_COND_MSG(!icon_map.has(p_name), "Cannot clear the icon '" + String(p_name) + "' because it does not exist.");
	icon_map.erase(p_name);
}

FileSystemAccess::FileSystemAccess() {
}

FileSystemAccess::~FileSystemAccess() {
}
