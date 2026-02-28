#pragma once

#include "core/object/ref_counted.h"

#include "scene/resources/texture.h"

#include "app/app_core/types/file_info.h"

#define COMPUTER_PATH "Computer"

#define UNPACK(...) __VA_ARGS__

#define FILE_SYSTEM_ACCESS_FUNC0(m_func_name)                                         \
protected:                                                                            \
	virtual void _##m_func_name() = 0;                                                \
                                                                                      \
public:                                                                               \
	static void m_func_name() {                                                       \
		ERR_FAIL_NULL_MSG(get_singleton(), "FileSystemAccess not instantiated yet."); \
		get_singleton()->_##m_func_name();                                            \
	}

#define FILE_SYSTEM_ACCESS_FUNC0_V(m_ret_type, m_retval, m_func_name)                             \
protected:                                                                                        \
	virtual m_ret_type _##m_func_name() = 0;                                                      \
                                                                                                  \
public:                                                                                           \
	static m_ret_type m_func_name() {                                                             \
		ERR_FAIL_NULL_V_MSG(get_singleton(), m_retval, "FileSystemAccess not instantiated yet."); \
		return get_singleton()->_##m_func_name();                                                 \
	}

#define FILE_SYSTEM_ACCESS_FUNC1_V(m_ret_type, m_retval, m_func_name, m_arg1_type, m_arg1)        \
protected:                                                                                        \
	virtual m_ret_type _##m_func_name(UNPACK m_arg1_type p_##m_arg1) = 0;                         \
                                                                                                  \
public:                                                                                           \
	static m_ret_type m_func_name(UNPACK m_arg1_type p_##m_arg1) {                                \
		ERR_FAIL_NULL_V_MSG(get_singleton(), m_retval, "FileSystemAccess not instantiated yet."); \
		return get_singleton()->_##m_func_name(p_##m_arg1);                                       \
	}

#define FILE_SYSTEM_ACCESS_FUNC2_V(m_ret_type, m_retval, m_func_name, m_arg1_type, m_arg1, m_arg2_type, m_arg2) \
protected:                                                                                                      \
	virtual m_ret_type _##m_func_name(UNPACK m_arg1_type p_##m_arg1, UNPACK m_arg2_type p_##m_arg2) = 0;        \
                                                                                                                \
public:                                                                                                         \
	static m_ret_type m_func_name(UNPACK m_arg1_type p_##m_arg1, UNPACK m_arg2_type p_##m_arg2) {               \
		ERR_FAIL_NULL_V_MSG(get_singleton(), m_retval, "FileSystemAccess not instantiated yet.");               \
		return get_singleton()->_##m_func_name(p_##m_arg1, p_##m_arg2);                                         \
	}

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
#if 0 // TODO: handle DRIVE_TYPE
	virtual Error _list_file_infos(const String &p_dir, List<FileInfo> &r_subdirs, List<FileInfo> &r_files, FileSortOption p_file_sort = FileSortOption::FILE_SORT_NAME) const = 0;
#endif
	virtual Error _list_drives(List<FileInfo> &r_drives) const = 0;

	virtual bool _path_exists(const String &p_file) const = 0;

	virtual bool _cut(const Vector<String> &p_files) = 0;
	virtual bool _copy(const Vector<String> &p_files) = 0;

public:
	static FileSystemAccess *get_singleton();

	template <typename T>
	static void make_default() {
		create_func = _create_builtin<T>;
	}

	static void create();
	static void destroy();

	// static bool is_link(String p_file);

	// Support absolute paths only.

	FILE_SYSTEM_ACCESS_FUNC1_V(Error, FAILED, list_dir_begin, (const String &), path);
	FILE_SYSTEM_ACCESS_FUNC1_V(Error, FAILED, get_next, (FileInfo &), info);
	FILE_SYSTEM_ACCESS_FUNC0(list_dir_end);

	static Ref<Texture2D> get_icon(const String &p_file_path, bool p_is_dir = false, bool p_is_hidden = false);
	FILE_SYSTEM_ACCESS_FUNC2_V(Error, FAILED, get_file_info, (const String &), file_path, (FileInfo &), info);
#if 0 // TODO: handle DRIVE_TYPE
	static Error list_file_infos(const String &p_dir, List<FileInfo> &r_subdirs, List<FileInfo> &r_files, FileSortOption p_file_sort = FileSortOption::FILE_SORT_NAME);
#endif
	static Error list_drives(List<FileInfo> &r_drives);

	static Error make_dir(const String &p_dir);
	static Error make_dir_recursive(const String &p_dir);

	static bool path_exists(const String &p_path);
	static bool file_exists(const String &p_file);
	static bool dir_exists(const String &p_dir);

	static bool is_root_type(const FileInfo &p_info);
	static bool is_drive_type(const FileInfo &p_info);
	static bool is_dir_type(const FileInfo &p_info);
	static bool is_root_type(const String &p_type);
	static bool is_drive_type(const String &p_type);
	static bool is_dir_type(const String &p_type);

	// TODO: return Error
	static bool cut(const Vector<String> &p_files);
	static bool copy(const Vector<String> &p_files);
	FILE_SYSTEM_ACCESS_FUNC0_V(bool, false, can_paste);
	FILE_SYSTEM_ACCESS_FUNC2_V(bool, false, get_clipboard_paths, (Vector<String> &), paths, (bool &), is_cut);
	FILE_SYSTEM_ACCESS_FUNC2_V(bool, false, paste, (const String &), dir, (Vector<String> &), dest_paths);

	static Error rename(const String &p_from, const String &p_to);
	static Error remove(const String &p_path);

	FILE_SYSTEM_ACCESS_FUNC2_V(Error, FAILED, create_file, (const String &), dir, (const String &), filename);

	// Icon cache
	static void set_system_icon(const StringName &p_name, const Ref<Texture2D> &p_icon);
	static Ref<Texture2D> get_system_icon(const StringName &p_name);
	static bool has_system_icon(const StringName &p_name);
	static void clear_system_icon(const StringName &p_name);

	FileSystemAccess();
	~FileSystemAccess();
};
