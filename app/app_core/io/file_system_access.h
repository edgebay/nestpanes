#pragma once

#include "core/object/ref_counted.h"

#include "scene/resources/texture.h"

#include "app/app_core/types/file_info.h"

#define COMPUTER_PATH "Computer"

class FileSystemAccess : public RefCounted {
	GDCLASS(FileSystemAccess, RefCounted);

public:
	typedef Ref<FileSystemAccess> (*CreateFunc)();

private:
	static HashMap<StringName, Ref<Texture2D>> icon_map;

	static CreateFunc create_func; ///< set this to instance a filesystem object

	static Ref<FileSystemAccess> singleton;

protected:
	// static void _bind_methods();

	template <typename T>
	static Ref<FileSystemAccess> _create_builtin() {
		return memnew(T);
	}

	virtual Ref<Texture2D> _get_icon(const String &p_file_path, bool p_is_dir = false, bool p_is_hidden = false) const = 0;
	virtual Error _get_file_info(const String &p_file_path, FileInfo &r_info) const = 0;
	virtual Error _list_file_infos(const String &p_dir, List<FileInfo> &r_subdirs, List<FileInfo> &r_files, FileSortOption p_file_sort = FileSortOption::FILE_SORT_NAME) const = 0;
	virtual Error _list_drives(List<FileInfo> &r_drives) const = 0;
	virtual Error _make_dir(const String &p_dir) = 0;
	virtual Error _make_dir_recursive(const String &p_dir);
	virtual bool _path_exists(const String &p_file) const = 0;
	virtual bool _file_exists(const String &p_file) const = 0;
	virtual bool _dir_exists(const String &p_dir) const = 0;

	virtual bool _open_in_terminal(const String &p_path) = 0;

	virtual bool _cut(const Vector<String> &p_files) = 0;
	virtual bool _copy(const Vector<String> &p_files) = 0;
	virtual bool _paste(const String &p_dir) = 0;

	virtual Error _rename(const String &p_path, const String &p_new_path) = 0;
	virtual Error _remove(const String &p_path) = 0;

	virtual bool _new_file(const String &p_dir, const String &p_filename) = 0;

public:
	virtual Error change_path(const String &p_dir) = 0; ///< can be relative or absolute, return false on success
	virtual String get_current_path() const = 0; ///< return current dir location

	static FileSystemAccess *get_singleton();

	template <typename T>
	static void make_default() {
		create_func = _create_builtin<T>;
	}

	static void create();
	static void destroy();

	// static bool is_link(String p_file);

	// Support absolute paths only.

	static Ref<Texture2D> get_icon(const String &p_file_path, bool p_is_dir = false, bool p_is_hidden = false);
	static Error get_file_info(const String &p_file_path, FileInfo &r_info);
	static Error list_file_infos(const String &p_dir, List<FileInfo> &r_subdirs, List<FileInfo> &r_files, FileSortOption p_file_sort = FileSortOption::FILE_SORT_NAME);
	static Error list_drives(List<FileInfo> &r_drives);

	static Error make_dir(const String &p_dir);
	static Error make_dir_recursive(const String &p_dir);

	static bool path_exists(const String &p_path);
	static bool file_exists(const String &p_file);
	static bool dir_exists(const String &p_dir);

	// TODO: return Error
	static bool open_in_terminal(const String &p_path);

	static bool cut(const Vector<String> &p_files);
	static bool copy(const Vector<String> &p_files);
	// TODO: static bool can_paste();?
	static bool paste(const String &p_dir);

	static Error rename(const String &p_path, const String &p_new_path);
	static Error remove(const String &p_path);

	static bool new_file(const String &p_dir, const String &p_filename);

	// Icon cache
	static void set_system_icon(const StringName &p_name, const Ref<Texture2D> &p_icon);
	static Ref<Texture2D> get_system_icon(const StringName &p_name);
	static bool has_system_icon(const StringName &p_name);
	static void clear_system_icon(const StringName &p_name);

	FileSystemAccess();
	~FileSystemAccess();
};
