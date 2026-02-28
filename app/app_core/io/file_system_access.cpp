#include "file_system_access.h"

#include "core/io/dir_access.h"

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
	icon_map.clear();
	singleton = Ref<FileSystemAccess>();
}

Ref<Texture2D> FileSystemAccess::get_icon(const String &p_file_path, bool p_is_dir, bool p_is_hidden) {
	ERR_FAIL_NULL_V_MSG(FileSystemAccess::get_singleton(), Ref<Texture2D>(), "FileSystemAccess not instantiated yet.");
	return FileSystemAccess::get_singleton()->_get_icon(p_file_path, p_is_dir, p_is_hidden);
}

#if 0 // TODO: handle DRIVE_TYPE
Error FileSystemAccess::list_file_infos(const String &p_dir, List<FileInfo> &r_subdirs, List<FileInfo> &r_files, FileSortOption p_file_sort) {
	ERR_FAIL_NULL_V_MSG(FileSystemAccess::get_singleton(), FAILED, "FileSystemAccess not instantiated yet.");
	return FileSystemAccess::get_singleton()->_list_file_infos(p_dir, r_subdirs, r_files, p_file_sort);
}
#endif

Error FileSystemAccess::list_drives(List<FileInfo> &r_drives) {
	ERR_FAIL_NULL_V_MSG(FileSystemAccess::get_singleton(), FAILED, "FileSystemAccess not instantiated yet.");
	return FileSystemAccess::get_singleton()->_list_drives(r_drives);
}

Error FileSystemAccess::make_dir(const String &p_dir) {
	if (dir_exists(p_dir)) {
		return OK;
	}
	return DirAccess::make_dir_absolute(p_dir);
}

Error FileSystemAccess::make_dir_recursive(const String &p_dir) {
	return DirAccess::make_dir_recursive_absolute(p_dir);
}

bool FileSystemAccess::path_exists(const String &p_path) {
	ERR_FAIL_NULL_V_MSG(FileSystemAccess::get_singleton(), false, "FileSystemAccess not instantiated yet.");
	return FileSystemAccess::get_singleton()->_path_exists(p_path);
}

bool FileSystemAccess::file_exists(const String &p_file) {
	Ref<DirAccess> d = DirAccess::create_for_path(p_file);
	return d->file_exists(p_file);
}

bool FileSystemAccess::dir_exists(const String &p_dir) {
	return DirAccess::dir_exists_absolute(p_dir);
}

bool FileSystemAccess::is_root_type(const FileInfo &p_info) {
	return is_root_type(p_info.type);
}

bool FileSystemAccess::is_drive_type(const FileInfo &p_info) {
	return is_drive_type(p_info.type);
}

bool FileSystemAccess::is_dir_type(const FileInfo &p_info) {
	return is_dir_type(p_info.type);
}

bool FileSystemAccess::is_root_type(const String &p_type) {
	return p_type == ROOT_TYPE;
}

bool FileSystemAccess::is_drive_type(const String &p_type) {
	return p_type == DRIVE_TYPE;
}

bool FileSystemAccess::is_dir_type(const String &p_type) {
	return p_type == FOLDER_TYPE || p_type == DRIVE_TYPE || p_type == ROOT_TYPE;
}

bool FileSystemAccess::cut(const Vector<String> &p_files) {
	ERR_FAIL_NULL_V_MSG(FileSystemAccess::get_singleton(), FAILED, "FileSystemAccess not instantiated yet.");
	return FileSystemAccess::get_singleton()->_cut(p_files);
}

bool FileSystemAccess::copy(const Vector<String> &p_files) {
	ERR_FAIL_NULL_V_MSG(FileSystemAccess::get_singleton(), FAILED, "FileSystemAccess not instantiated yet.");
	return FileSystemAccess::get_singleton()->_copy(p_files);
}

Error FileSystemAccess::rename(const String &p_from, const String &p_to) {
	return DirAccess::rename_absolute(p_from, p_to);
}

Error FileSystemAccess::remove(const String &p_path) {
	return DirAccess::remove_absolute(p_path);
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
